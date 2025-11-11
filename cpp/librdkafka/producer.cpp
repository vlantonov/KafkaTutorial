#include <librdkafka/rdkafka.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

// Delivery report callback
static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage,
                      void *opaque) {
  if (rkmessage->err) {
    cerr << "Message delivery failed: " << rd_kafka_err2str(rkmessage->err)
         << endl;
  } else {
    cout << "Message delivered to partition " << rkmessage->partition
         << " at offset " << rkmessage->offset << endl;
  }
}

int main(int argc, char *argv[]) {
  // Kafka broker address
  string brokers = "localhost:9092";
  string topic_name = "test-topic";

  if (argc > 1) {
    brokers = argv[1];
  }
  if (argc > 2) {
    topic_name = argv[2];
  }

  // Configuration
  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  char errstr[512];

  // Set bootstrap servers
  if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(), errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK) {
    cerr << "Failed to set bootstrap.servers: " << errstr << endl;
    rd_kafka_conf_destroy(conf);
    return 1;
  }

  // Set delivery report callback
  rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

  // Create producer instance
  rd_kafka_t *producer =
      rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!producer) {
    cerr << "Failed to create producer: " << errstr << endl;
    rd_kafka_conf_destroy(conf);
    return 1;
  }

  cout << "Kafka Producer started. Broker: " << brokers << endl;
  cout << "Topic: " << topic_name << endl;
  cout << "Type messages (Ctrl+C to exit):" << endl;

  int msg_count = 0;
  string line;

  while (getline(cin, line)) {
    if (line.empty()) continue;

    string key = to_string(msg_count);

    // Produce message
    rd_kafka_resp_err_t err = rd_kafka_producev(
        producer, RD_KAFKA_V_TOPIC(topic_name.c_str()),
        RD_KAFKA_V_KEY(key.c_str(), key.size()),
        RD_KAFKA_V_VALUE(line.data(), line.size()),
        RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY), RD_KAFKA_V_END);

    if (err) {
      cerr << "Failed to produce message: " << rd_kafka_err2str(err) << endl;
    } else {
      cout << "Queued message #" << msg_count << ": " << line << endl;
    }

    msg_count++;

    // Poll for delivery reports
    rd_kafka_poll(producer, 0);
  }

  // Wait for all messages to be delivered
  cout << "\nFlushing messages..." << endl;
  rd_kafka_flush(producer, 10000);  // 10 second timeout

  cout << "Total messages sent: " << msg_count << endl;

  // Cleanup
  rd_kafka_destroy(producer);

  return 0;
}
