#include <librdkafka/rdkafka.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

static volatile sig_atomic_t run = 1;

static void stop(int) { run = 0; }

namespace {

void print_partitions(const rd_kafka_topic_partition_list_t *partitions) {
  for (int i = 0; i < partitions->cnt; ++i) {
    const auto *p = &partitions->elems[i];
    cout << p->topic << "[" << p->partition << "]";
    if (i + 1 < partitions->cnt) {
      cout << ", ";
    }
  }
}

}  // namespace

static void rebalance_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err,
                         rd_kafka_topic_partition_list_t *partitions,
                         void *) {
  switch (err) {
    case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
      cout << "[REBALANCE] ASSIGNED " << partitions->cnt << " partition(s): ";
      print_partitions(partitions);
      cout << endl;
      rd_kafka_assign(rk, partitions);
      break;

    case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
      cout << "[REBALANCE] REVOKED " << partitions->cnt << " partition(s): ";
      print_partitions(partitions);
      cout << endl;
      rd_kafka_assign(rk, nullptr);
      break;

    default:
      cerr << "[REBALANCE] ERROR: " << rd_kafka_err2str(err) << endl;
      rd_kafka_assign(rk, nullptr);
      break;
  }
}

int main(int argc, char *argv[]) {
  string brokers = "localhost:9092";
  string topic_name = "test-topic";
  string group_id = "rebalance-demo-group";

  if (argc > 1) {
    brokers = argv[1];
  }
  if (argc > 2) {
    topic_name = argv[2];
  }
  if (argc > 3) {
    group_id = argv[3];
  }

  signal(SIGINT, stop);
  signal(SIGTERM, stop);

  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  char errstr[512];

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

  rd_kafka_conf_set_rebalance_cb(conf, rebalance_cb);

  rd_kafka_t *consumer =
      rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
  if (!consumer) {
    cerr << "Failed to create consumer: " << errstr << endl;
    rd_kafka_conf_destroy(conf);
    return 1;
  }

  rd_kafka_topic_partition_list_t *topics =
      rd_kafka_topic_partition_list_new(1);
  rd_kafka_topic_partition_list_add(topics, topic_name.c_str(),
                                    RD_KAFKA_PARTITION_UA);

  rd_kafka_resp_err_t subscribe_err = rd_kafka_subscribe(consumer, topics);
  rd_kafka_topic_partition_list_destroy(topics);
  if (subscribe_err) {
    cerr << "Failed to subscribe: " << rd_kafka_err2str(subscribe_err) << endl;
    rd_kafka_destroy(consumer);
    return 1;
  }

  cout << "Rebalance demo consumer started" << endl;
  cout << "Broker: " << brokers << endl;
  cout << "Topic: " << topic_name << endl;
  cout << "Group ID: " << group_id << endl;
  cout << "Start a second instance to trigger partition reassignment."
       << endl;
  cout << "Waiting for messages (Ctrl+C to exit)..." << endl << endl;

  while (run) {
    rd_kafka_message_t *message = rd_kafka_consumer_poll(consumer, 1000);
    if (!message) {
      continue;
    }

    if (message->err) {
      if (message->err != RD_KAFKA_RESP_ERR__PARTITION_EOF) {
        cerr << "Consumer error: " << rd_kafka_message_errstr(message) << endl;
      }
      rd_kafka_message_destroy(message);
      continue;
    }

    cout << "[MESSAGE] " << rd_kafka_topic_name(message->rkt) << "["
         << message->partition << "]@" << message->offset;
    if (message->key) {
      cout << " key=" << string(static_cast<const char *>(message->key),
                                 message->key_len);
    }
    if (message->payload) {
      cout << " value=" << string(static_cast<const char *>(message->payload),
                                   message->len);
    }
    cout << endl;

    rd_kafka_message_destroy(message);
  }

  cout << "\nShutting down rebalance demo consumer..." << endl;
  rd_kafka_consumer_close(consumer);
  rd_kafka_destroy(consumer);
  return 0;
}
