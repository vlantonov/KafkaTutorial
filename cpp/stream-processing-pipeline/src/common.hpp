// Shared helpers for the C++ stream-processing pipeline demo.
#pragma once

#include <librdkafka/rdkafkacpp.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace spp {

// Read an environment variable, returning a default when it is unset/empty.
inline std::string env(const std::string &name, const std::string &fallback) {
  const char *value = std::getenv(name.c_str());
  if (value == nullptr || value[0] == '\0') {
    return fallback;
  }
  return std::string(value);
}

inline int env_int(const std::string &name, int fallback) {
  const std::string value = env(name, "");
  if (value.empty()) {
    return fallback;
  }
  return std::stoi(value);
}

// Current wall-clock time in milliseconds since the epoch.
inline int64_t now_ms() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

// Thread-safe, timestamped logger so interleaved container logs stay readable.
inline void log(const std::string &tag, const std::string &message) {
  static std::mutex mu;
  std::lock_guard<std::mutex> guard(mu);
  std::cout << "[" << tag << "] " << message << std::endl;
}

// A single event flowing through the pipeline.
struct Event {
  std::string id;    // idempotency key used for deduplication
  std::string key;   // grouping key (e.g. sensor id)
  int64_t ts_ms;     // event-time timestamp
  double value;      // measured value
};

// Serialize an event as CSV: id,key,ts_ms,value
inline std::string to_csv(const Event &e) {
  std::ostringstream os;
  os << e.id << ',' << e.key << ',' << e.ts_ms << ',' << e.value;
  return os.str();
}

// Parse an event from CSV. Returns false on malformed input.
inline bool from_csv(const std::string &line, Event &out) {
  std::istringstream is(line);
  std::string ts, value;
  if (!std::getline(is, out.id, ',')) return false;
  if (!std::getline(is, out.key, ',')) return false;
  if (!std::getline(is, ts, ',')) return false;
  if (!std::getline(is, value)) return false;
  try {
    out.ts_ms = std::stoll(ts);
    out.value = std::stod(value);
  } catch (const std::exception &) {
    return false;
  }
  return true;
}

// Delivery report callback that only surfaces failures.
class FailureReporter : public RdKafka::DeliveryReportCb {
 public:
  explicit FailureReporter(std::string tag) : tag_(std::move(tag)) {}
  void dr_cb(RdKafka::Message &message) override {
    if (message.err()) {
      log(tag_, std::string("delivery failed: ") + message.errstr());
    }
  }

 private:
  std::string tag_;
};

}  // namespace spp
