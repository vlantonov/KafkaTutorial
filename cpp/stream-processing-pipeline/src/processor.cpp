// Stream processor demonstrating four streaming concerns together:
//
//   * Windowing     - tumbling event-time windows aggregate per-key stats.
//   * Deduplication - repeated event ids are dropped using bounded state.
//   * Backpressure  - an ingestion thread feeds a bounded queue; when the
//                     processing thread falls behind, the queue fills and the
//                     consumer is paused, then resumed once it drains.
//   * Exactly-once  - each flush emits window results AND commits the source
//                     offsets inside a single Kafka transaction (EOS), so
//                     output and progress advance atomically.
//
// Splitting ingestion from processing is what makes backpressure and
// event-time windowing coexist correctly: pausing the consumer stops new
// reads, but the processing thread keeps draining already-buffered events, so
// the watermark advances and windows still close (and are emitted exactly
// once) instead of being flushed while incomplete.
#include <librdkafka/rdkafkacpp.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common.hpp"

using namespace spp;

namespace {

struct Aggregate {
  int64_t count = 0;
  double sum = 0.0;
};

struct WindowKey {
  std::string key;
  int64_t window_start;
  bool operator<(const WindowKey &o) const {
    if (window_start != o.window_start) return window_start < o.window_start;
    return key < o.key;
  }
};

struct QueueItem {
  Event ev;
  int32_t partition;
  int64_t offset;
};

// Shared state between the ingestion and processing threads.
struct Shared {
  std::mutex qmu;
  std::condition_variable qcv;
  std::deque<QueueItem> queue;
  std::mutex cmu;  // guards consumer pause/resume/assignment/groupMetadata
  std::atomic<bool> running{true};
  std::atomic<bool> source_idle{false};  // set at end-of-stream
};

}  // namespace

int main() {
  const std::string brokers = env("BROKERS", "kafka:9092");
  const std::string input_topic = env("INPUT_TOPIC", "events-input");
  const std::string output_topic = env("OUTPUT_TOPIC", "events-windowed");
  const std::string group_id = env("GROUP_ID", "spp-processor");
  const std::string txn_id = env("TRANSACTIONAL_ID", "spp-processor-txn");
  const int64_t window_ms = env_int("WINDOW_MS", 5000);
  const int64_t allowed_lateness_ms = env_int("ALLOWED_LATENESS_MS", 2000);
  const int64_t commit_interval_ms = env_int("COMMIT_INTERVAL_MS", 1000);
  const size_t queue_high = env_int("MAX_BUFFERED", 150);   // pause here
  const size_t queue_low = env_int("RESUME_BUFFERED", 50);  // resume here
  const int64_t idle_flush_ms = env_int("IDLE_FLUSH_MS", 4000);
  const int process_cost_ms = env_int("PROCESS_COST_MS", 0);  // simulated work

  std::string errstr;

  // ---- Consumer (source) -------------------------------------------------
  std::unique_ptr<RdKafka::Conf> cconf(
      RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  cconf->set("bootstrap.servers", brokers, errstr);
  cconf->set("group.id", group_id, errstr);
  cconf->set("enable.auto.commit", "false", errstr);  // offsets via txn only
  cconf->set("auto.offset.reset", "earliest", errstr);
  cconf->set("isolation.level", "read_committed", errstr);
  std::unique_ptr<RdKafka::KafkaConsumer> consumer(
      RdKafka::KafkaConsumer::create(cconf.get(), errstr));
  if (!consumer) {
    log("processor", "failed to create consumer: " + errstr);
    return 1;
  }
  consumer->subscribe({input_topic});

  // ---- Producer (sink, transactional) ------------------------------------
  std::unique_ptr<RdKafka::Conf> pconf(
      RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  pconf->set("bootstrap.servers", brokers, errstr);
  pconf->set("transactional.id", txn_id, errstr);  // enables EOS + idempotence
  FailureReporter reporter("processor");
  pconf->set("dr_cb", &reporter, errstr);
  std::unique_ptr<RdKafka::Producer> producer(
      RdKafka::Producer::create(pconf.get(), errstr));
  if (!producer) {
    log("processor", "failed to create producer: " + errstr);
    return 1;
  }
  if (RdKafka::Error *e = producer->init_transactions(30000)) {
    log("processor", "init_transactions failed: " + e->str());
    delete e;
    return 1;
  }

  log("processor", "started. window=" + std::to_string(window_ms) +
                       "ms lateness=" + std::to_string(allowed_lateness_ms) +
                       "ms queue(high/low)=" + std::to_string(queue_high) + "/" +
                       std::to_string(queue_low));

  Shared shared;

  // ---- Ingestion thread: consume -> bounded queue, pause on backpressure --
  std::thread ingest([&]() {
    bool paused = false;
    int64_t last_msg = now_ms();
    while (shared.running.load()) {
      std::unique_ptr<RdKafka::Message> msg(consumer->consume(200));
      const int64_t now = now_ms();

      if (msg->err() == RdKafka::ERR_NO_ERROR) {
        last_msg = now;
        shared.source_idle.store(false);
        Event e;
        if (msg->payload() &&
            from_csv(std::string(static_cast<const char *>(msg->payload()),
                                 msg->len()),
                     e)) {
          size_t size;
          {
            std::lock_guard<std::mutex> lk(shared.qmu);
            shared.queue.push_back({e, msg->partition(), msg->offset()});
            size = shared.queue.size();
          }
          shared.qcv.notify_one();
          if (!paused && size >= queue_high) {
            std::lock_guard<std::mutex> lk(shared.cmu);
            std::vector<RdKafka::TopicPartition *> a;
            consumer->assignment(a);
            consumer->pause(a);
            for (auto *tp : a) delete tp;
            paused = true;
            log("processor", "PAUSE consumption (backpressure, queue=" +
                                 std::to_string(size) + ")");
          }
        }
      } else if (msg->err() == RdKafka::ERR__TIMED_OUT ||
                 msg->err() == RdKafka::ERR__PARTITION_EOF) {
        if (paused) {
          size_t size;
          {
            std::lock_guard<std::mutex> lk(shared.qmu);
            size = shared.queue.size();
          }
          if (size <= queue_low) {
            std::lock_guard<std::mutex> lk(shared.cmu);
            std::vector<RdKafka::TopicPartition *> a;
            consumer->assignment(a);
            consumer->resume(a);
            for (auto *tp : a) delete tp;
            paused = false;
            log("processor", "RESUME consumption (queue drained to " +
                                 std::to_string(size) + ")");
          }
        } else if (now - last_msg >= idle_flush_ms) {
          shared.source_idle.store(true);  // true end-of-stream
        }
      } else {
        log("processor", "consume error: " + msg->errstr());
      }
    }
  });

  // ---- Processing thread: windowing + dedup + transactional flush --------
  std::map<WindowKey, Aggregate> windows;
  std::unordered_set<std::string> seen_ids;
  std::map<int64_t, std::vector<std::string>> ids_by_window;
  std::unordered_map<int32_t, int64_t> next_offsets;
  int64_t watermark = 0;
  bool in_txn = false;
  int64_t last_commit = now_ms();
  int64_t total_out = 0;
  int64_t total_dupes = 0;
  int64_t total_late = 0;

  auto begin_txn_if_needed = [&]() -> bool {
    if (in_txn) return true;
    if (RdKafka::Error *e = producer->begin_transaction()) {
      log("processor", "begin_transaction failed: " + e->str());
      delete e;
      return false;
    }
    in_txn = true;
    return true;
  };

  // Emit windows whose end <= close_before and atomically commit offsets.
  auto flush = [&](int64_t close_before) {
    std::vector<std::map<WindowKey, Aggregate>::iterator> closed;
    int64_t max_end = 0;
    for (auto it = windows.begin(); it != windows.end(); ++it) {
      const int64_t window_end = it->first.window_start + window_ms;
      if (window_end <= close_before) {
        closed.push_back(it);
        max_end = std::max(max_end, window_end);
      }
    }
    if (closed.empty() && next_offsets.empty()) return;
    if (!begin_txn_if_needed()) return;

    for (auto it : closed) {
      const WindowKey &wk = it->first;
      const Aggregate &agg = it->second;
      const int64_t window_end = wk.window_start + window_ms;
      const double avg = agg.count ? agg.sum / agg.count : 0.0;
      std::ostringstream os;
      os << wk.key << ',' << wk.window_start << ',' << window_end << ','
         << agg.count << ',' << agg.sum << ',' << avg;
      const std::string payload = os.str();
      producer->produce(output_topic, RdKafka::Topic::PARTITION_UA,
                        RdKafka::Producer::RK_MSG_COPY,
                        const_cast<char *>(payload.data()), payload.size(),
                        wk.key.data(), wk.key.size(), 0, nullptr, nullptr);
      ++total_out;
      auto exp = ids_by_window.find(wk.window_start);
      if (exp != ids_by_window.end()) {
        for (const auto &id : exp->second) seen_ids.erase(id);
        ids_by_window.erase(exp);
      }
      windows.erase(it);
    }
    // Once emitted, no later event may reopen these windows.
    watermark = std::max(watermark, max_end);

    std::vector<RdKafka::TopicPartition *> offsets;
    for (const auto &kv : next_offsets) {
      offsets.push_back(
          RdKafka::TopicPartition::create(input_topic, kv.first, kv.second));
    }
    RdKafka::ConsumerGroupMetadata *md;
    {
      std::lock_guard<std::mutex> lk(shared.cmu);
      md = consumer->groupMetadata();
    }
    if (RdKafka::Error *e =
            producer->send_offsets_to_transaction(offsets, md, 30000)) {
      log("processor", "send_offsets failed: " + e->str());
      delete e;
    }
    RdKafka::Error *commit_err = producer->commit_transaction(30000);
    in_txn = false;
    if (commit_err) {
      log("processor", "commit_transaction failed: " + commit_err->str());
      delete commit_err;
    }
    delete md;
    for (auto *tp : offsets) delete tp;

    if (!closed.empty()) {
      log("processor",
          "committed txn: emitted " + std::to_string(closed.size()) +
              " window(s), total_windows=" + std::to_string(total_out) +
              ", dupes_dropped=" + std::to_string(total_dupes) +
              ", late_dropped=" + std::to_string(total_late));
    }
  };

  while (shared.running.load()) {
    std::vector<QueueItem> batch;
    {
      std::unique_lock<std::mutex> lk(shared.qmu);
      shared.qcv.wait_for(lk, std::chrono::milliseconds(100), [&]() {
        return !shared.queue.empty() || !shared.running.load();
      });
      while (!shared.queue.empty() && batch.size() < 500) {
        batch.push_back(std::move(shared.queue.front()));
        shared.queue.pop_front();
      }
    }

    for (auto &item : batch) {
      const Event &e = item.ev;
      next_offsets[item.partition] = item.offset + 1;
      watermark = std::max(watermark, e.ts_ms - allowed_lateness_ms);
      if (seen_ids.count(e.id)) {
        ++total_dupes;
        continue;
      }
      const int64_t window_start = (e.ts_ms / window_ms) * window_ms;
      if (window_start + window_ms <= watermark) {
        ++total_late;  // arrived after its window already closed
        continue;
      }
      seen_ids.insert(e.id);
      ids_by_window[window_start].push_back(e.id);
      Aggregate &agg = windows[WindowKey{e.key, window_start}];
      agg.count += 1;
      agg.sum += e.value;
      if (process_cost_ms > 0) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(process_cost_ms));  // slow sink
      }
    }

    const int64_t now = now_ms();
    if (now - last_commit >= commit_interval_ms) {
      flush(watermark);
      last_commit = now;
    }

    bool empty;
    {
      std::lock_guard<std::mutex> lk(shared.qmu);
      empty = shared.queue.empty();
    }
    if (shared.source_idle.load() && empty && !windows.empty()) {
      log("processor", "source drained, closing remaining windows");
      flush(std::numeric_limits<int64_t>::max());
    }
  }

  shared.running.store(false);
  shared.qcv.notify_all();
  ingest.join();
  return 0;
}
