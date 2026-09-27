// Producer: generates a finite stream of events, deliberately injecting
// duplicates and late events so the downstream processor's deduplication and
// windowing can be observed. Uses an idempotent producer for clean delivery.
#include <librdkafka/rdkafkacpp.h>

#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"

using namespace spp;

int main() {
  const std::string brokers = env("BROKERS", "kafka:9092");
  const std::string topic = env("INPUT_TOPIC", "events-input");
  const int num_events = env_int("NUM_EVENTS", 200);
  const int dup_percent = env_int("DUP_PERCENT", 15);   // resend probability
  const int late_percent = env_int("LATE_PERCENT", 10);  // out-of-order events
  const int event_ms = env_int("EVENT_INTERVAL_MS", 50);  // event-time step
  const int delay_ms = env_int("PRODUCE_DELAY_MS", 20);   // real-time pacing

  const std::vector<std::string> keys = {"sensor-a", "sensor-b", "sensor-c"};

  std::string errstr;
  std::unique_ptr<RdKafka::Conf> conf(
      RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  conf->set("bootstrap.servers", brokers, errstr);
  conf->set("enable.idempotence", "true", errstr);
  conf->set("acks", "all", errstr);

  FailureReporter reporter("producer");
  conf->set("dr_cb", &reporter, errstr);

  std::unique_ptr<RdKafka::Producer> producer(
      RdKafka::Producer::create(conf.get(), errstr));
  if (!producer) {
    log("producer", "failed to create producer: " + errstr);
    return 1;
  }

  log("producer", "producing " + std::to_string(num_events) + " events to '" +
                      topic + "' via " + brokers);

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> pct(1, 100);
  std::uniform_int_distribution<int> key_pick(0, keys.size() - 1);
  std::uniform_real_distribution<double> val(10.0, 40.0);
  std::uniform_int_distribution<int> lateness(1, 5);

  int64_t event_time = now_ms();
  int produced = 0;
  int duplicates = 0;

  for (int i = 0; i < num_events; ++i) {
    Event e;
    e.id = "evt-" + std::to_string(i);
    e.key = keys[key_pick(rng)];
    e.value = val(rng);
    e.ts_ms = event_time;
    if (pct(rng) <= late_percent) {
      // Emit an out-of-order (late) timestamp within the allowed lateness.
      e.ts_ms -= static_cast<int64_t>(lateness(rng)) * event_ms;
    }

    auto emit = [&](const Event &ev) {
      const std::string payload = to_csv(ev);
      RdKafka::ErrorCode err = producer->produce(
          topic, RdKafka::Topic::PARTITION_UA,
          RdKafka::Producer::RK_MSG_COPY,
          const_cast<char *>(payload.data()), payload.size(),
          ev.key.data(), ev.key.size(), 0, nullptr, nullptr);
      if (err != RdKafka::ERR_NO_ERROR) {
        log("producer", "produce failed: " + RdKafka::err2str(err));
      } else {
        ++produced;
      }
      producer->poll(0);
    };

    emit(e);

    // Occasionally resend the exact same event to exercise deduplication.
    if (pct(rng) <= dup_percent) {
      emit(e);
      ++duplicates;
    }

    event_time += event_ms;
    if (delay_ms > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
  }

  producer->flush(15000);
  log("producer", "done. records sent=" + std::to_string(produced) +
                      " (including " + std::to_string(duplicates) +
                      " duplicates)");
  return 0;
}
