#include <librdkafka/rdkafka.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

// Signal handler for graceful shutdown
static volatile sig_atomic_t run = 1;

static void stop(int sig) { run = 0; }

// Rebalance callback
static void rebalance_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err,
                         rd_kafka_topic_partition_list_t *partitions,
                         void *opaque) {
  cout << "Consumer group rebalanced: ";

  switch (err) {
    case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
      cout << "assigned " << partitions->cnt << " partition(s)" << endl;
      rd_kafka_assign(rk, partitions);
      break;

    case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
      cout << "revoked " << partitions->cnt << " partition(s)" << endl;
      rd_kafka_assign(rk, NULL);
      break;

    default:
      cerr << "failed: " << rd_kafka_err2str(err) << endl;
      rd_kafka_assign(rk, NULL);
      break;
  }
}

int main(int argc, char *argv[]) {
  // Kafka broker address
  string brokers = "localhost:9092";
  string topic_name = "test-topic";
  string group_id = "test-consumer-group";

  if (argc > 1) {
    brokers = argv[1];
  }
  if (argc > 2) {
    topic_name = argv[2];
  }
  if (argc > 3) {
    group_id = argv[3];
  }

  // Set up signal handlers
  signal(SIGINT, stop);
  signal(SIGTERM, stop);

  // Configuration
  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  char errstr[512];

  // Set configuration properties
  if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(), errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK ||
      rd_kafka_conf_set(conf, "group.id", group_id.c_str(), errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK ||
      rd_kafka_conf_set(conf, "auto.offset.reset", "earliest", errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK ||
      rd_kafka_conf_set(conf, "enable.auto.commit", "true", errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK) {
    cerr << "Configuration failed: " << errstr << endl;
    rd_kafka_conf_destroy(conf);
    return 1;
  }

  // Set rebalance callback
  rd_kafka_conf_set_rebalance_cb(conf, rebalance_cb);

  // Create consumer instance
  rd_kafka_t *consumer =
      rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
  if (!consumer) {
    cerr << "Failed to create consumer: " << errstr << endl;
    rd_kafka_conf_destroy(conf);
    return 1;
  }

  // Subscribe to topic
  rd_kafka_topic_partition_list_t *topics =
      rd_kafka_topic_partition_list_new(1);
  rd_kafka_topic_partition_list_add(topics, topic_name.c_str(),
                                    RD_KAFKA_PARTITION_UA);

  rd_kafka_resp_err_t err = rd_kafka_subscribe(consumer, topics);
  if (err) {
    cerr << "Failed to subscribe to topic: " << rd_kafka_err2str(err) << endl;
    rd_kafka_topic_partition_list_destroy(topics);
    rd_kafka_destroy(consumer);
    return 1;
  }

  rd_kafka_topic_partition_list_destroy(topics);

  cout << "Kafka Consumer started" << endl;
  cout << "Broker: " << brokers << endl;
  cout << "Topic: " << topic_name << endl;
  cout << "Group ID: " << group_id << endl;
  cout << "Waiting for messages (Ctrl+C to exit)..." << endl << endl;

  int msg_count = 0;

  // Poll for messages
  while (run) {
    rd_kafka_message_t *message = rd_kafka_consumer_poll(consumer, 1000);

    if (!message) {
      continue;  // Timeout, no message
    }

    if (message->err) {
      if (message->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
        // End of partition, not an error
      } else if (message->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
                 message->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC) {
        cerr << "Consumer error: " << rd_kafka_message_errstr(message) << endl;
      } else {
        cerr << "Consumer error: " << rd_kafka_message_errstr(message) << endl;
      }
    } else {
      // Process message
      cout << "Received message #" << msg_count++ << endl;
      cout << "  Topic: " << rd_kafka_topic_name(message->rkt) << endl;
      cout << "  Partition: " << message->partition << endl;
      cout << "  Offset: " << message->offset << endl;

      if (message->key) {
        cout << "  Key: "
             << string((const char *)message->key, message->key_len) << endl;
      }

      if (message->payload) {
        cout << "  Payload: "
             << string((const char *)message->payload, message->len) << endl;
      }

      cout << endl;
    }

    rd_kafka_message_destroy(message);
  }

  // Close consumer
  cout << "\nShutting down..." << endl;
  rd_kafka_consumer_close(consumer);
  rd_kafka_destroy(consumer);

  cout << "Total messages received: " << msg_count << endl;

  return 0;
}
