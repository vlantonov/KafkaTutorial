#include <librdkafka/rdkafka.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace {

bool set_conf_or_exit(rd_kafka_conf_t *conf, const string &name,
                      const string &value) {
  char errstr[512];
  if (rd_kafka_conf_set(conf, name.c_str(), value.c_str(), errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK) {
    cerr << "Failed to set " << name << ": " << errstr << endl;
    return false;
  }
  return true;
}

void append_uvarint(vector<uint8_t> &out, uint64_t value) {
  while (value >= 0x80) {
    out.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
    value >>= 7;
  }
  out.push_back(static_cast<uint8_t>(value));
}

void append_proto_string_field(vector<uint8_t> &out, uint32_t field_number,
                               const string &value) {
  append_uvarint(out, (field_number << 3U) | 2U);
  append_uvarint(out, value.size());
  out.insert(out.end(), value.begin(), value.end());
}

void append_proto_uint64_field(vector<uint8_t> &out, uint32_t field_number,
                               uint64_t value) {
  append_uvarint(out, field_number << 3U);
  append_uvarint(out, value);
}

vector<uint8_t> build_confluent_protobuf_payload(int schema_id,
                                                 const string &user_id,
                                                 const string &action,
                                                 uint64_t event_ts) {
  vector<uint8_t> proto_payload;
  proto_payload.reserve(64);
  append_proto_string_field(proto_payload, 1, user_id);
  append_proto_string_field(proto_payload, 2, action);
  append_proto_uint64_field(proto_payload, 3, event_ts);

  vector<uint8_t> payload;
  payload.reserve(proto_payload.size() + 8);
  payload.push_back(0);
  payload.push_back(static_cast<uint8_t>((schema_id >> 24) & 0xFF));
  payload.push_back(static_cast<uint8_t>((schema_id >> 16) & 0xFF));
  payload.push_back(static_cast<uint8_t>((schema_id >> 8) & 0xFF));
  payload.push_back(static_cast<uint8_t>(schema_id & 0xFF));
  payload.push_back(0);  // top-level message index [0]
  payload.insert(payload.end(), proto_payload.begin(), proto_payload.end());
  return payload;
}

}  // namespace

int main(int argc, char *argv[]) {
  string brokers = "localhost:9092";
  string topic = "proto-user-events";
  int schema_id = 1;

  if (argc > 1) {
    brokers = argv[1];
  }
  if (argc > 2) {
    topic = argv[2];
  }
  if (argc > 3) {
    schema_id = stoi(argv[3]);
  }

  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  char errstr[512];
  if (!set_conf_or_exit(conf, "bootstrap.servers", brokers)) {
    rd_kafka_conf_destroy(conf);
    return 1;
  }

  rd_kafka_t *producer =
      rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (producer == nullptr) {
    cerr << "Failed to create producer: " << errstr << endl;
    rd_kafka_conf_destroy(conf);
    return 1;
  }

  const string user_id = "u-200";
  const string action = "purchase";
  const uint64_t event_ts = 1735689600;
  vector<uint8_t> payload =
      build_confluent_protobuf_payload(schema_id, user_id, action, event_ts);

  rd_kafka_resp_err_t produce_err =
      rd_kafka_producev(producer, RD_KAFKA_V_TOPIC(topic.c_str()),
                        RD_KAFKA_V_KEY(user_id.c_str(), user_id.size()),
                        RD_KAFKA_V_VALUE(payload.data(), payload.size()),
                        RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                        RD_KAFKA_V_END);

  if (produce_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    cerr << "Failed to queue Protobuf message: " << rd_kafka_err2str(produce_err)
         << endl;
    rd_kafka_destroy(producer);
    return 1;
  }

  rd_kafka_flush(producer, 10000);
  cout << "Produced Protobuf message to topic '" << topic
       << "' with schema_id=" << schema_id << endl;
  rd_kafka_destroy(producer);
  return 0;
}
