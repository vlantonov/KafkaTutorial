// Verifier: reads the windowed output topic with read_committed isolation so
// only committed (exactly-once) transaction results are visible. It prints each
// window result and flags any duplicate (key, window_start) pair, which must
// never occur when exactly-once semantics hold.
#include <librdkafka/rdkafkacpp.h>

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "common.hpp"

using namespace spp;

int main() {
  const std::string brokers = env("BROKERS", "kafka:9092");
  const std::string topic = env("OUTPUT_TOPIC", "events-windowed");
  const std::string group_id = env("GROUP_ID", "spp-verifier");

  std::string errstr;
  std::unique_ptr<RdKafka::Conf> conf(
      RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  conf->set("bootstrap.servers", brokers, errstr);
  conf->set("group.id", group_id, errstr);
  conf->set("auto.offset.reset", "earliest", errstr);
  conf->set("isolation.level", "read_committed", errstr);  // EOS: committed only

  std::unique_ptr<RdKafka::KafkaConsumer> consumer(
      RdKafka::KafkaConsumer::create(conf.get(), errstr));
  if (!consumer) {
    log("verifier", "failed to create consumer: " + errstr);
    return 1;
  }
  consumer->subscribe({topic});
  log("verifier", "reading committed window results from '" + topic + "'");

  std::set<std::pair<std::string, std::string>> seen;  // (key, window_start)
  int64_t results = 0;
  int64_t duplicates = 0;

  while (true) {
    std::unique_ptr<RdKafka::Message> msg(consumer->consume(500));
    if (msg->err() != RdKafka::ERR_NO_ERROR) {
      continue;
    }
    const std::string line(static_cast<const char *>(msg->payload()),
                           msg->len());
    // line: key,window_start,window_end,count,sum,avg
    std::string key, window_start, rest;
    const auto p1 = line.find(',');
    const auto p2 = line.find(',', p1 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos) continue;
    key = line.substr(0, p1);
    window_start = line.substr(p1 + 1, p2 - p1 - 1);

    ++results;
    auto id = std::make_pair(key, window_start);
    const bool dup = !seen.insert(id).second;
    if (dup) {
      ++duplicates;
      log("verifier", "DUPLICATE window result (EOS violated!): " + line);
    } else {
      log("verifier", "window " + std::to_string(results) + ": " + line);
    }
    log("verifier", "totals: results=" + std::to_string(results) +
                        " duplicate_windows=" + std::to_string(duplicates));
  }

  return 0;
}
