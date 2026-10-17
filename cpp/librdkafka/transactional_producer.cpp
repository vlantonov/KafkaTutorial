#include <librdkafka/rdkafka.h>

#include <iostream>
#include <string>

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

void destroy_error(rd_kafka_error_t *error) {
  if (error != nullptr) {
    rd_kafka_error_destroy(error);
  }
}

}  // namespace

int main(int argc, char *argv[]) {
  string brokers = "localhost:9092";
  string topic_name = "test-topic";

  if (argc > 1) {
    brokers = argv[1];
  }
  if (argc > 2) {
    topic_name = argv[2];
  }

  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  char errstr[512];

  if (!set_conf_or_exit(conf, "bootstrap.servers", brokers) ||
      !set_conf_or_exit(conf, "enable.idempotence", "true") ||
      !set_conf_or_exit(conf, "acks", "all") ||
      !set_conf_or_exit(conf, "transactional.id",
                        "cpp-eos-producer-transaction-1")) {
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

  cout << "Initializing transactions..." << endl;
  rd_kafka_error_t *error = rd_kafka_init_transactions(producer, 10000);
  if (error != nullptr) {
    cerr << "init_transactions failed: " << rd_kafka_error_string(error) << endl;
    destroy_error(error);
    rd_kafka_destroy(producer);
    return 1;
  }
  destroy_error(error);

  cout << "Beginning transaction..." << endl;
  error = rd_kafka_begin_transaction(producer);
  if (error != nullptr) {
    cerr << "begin_transaction failed: " << rd_kafka_error_string(error) << endl;
    destroy_error(error);
    rd_kafka_destroy(producer);
    return 1;
  }
  destroy_error(error);

  bool produce_failed = false;
  bool transaction_failed = false;
  for (int i = 0; i < 5; ++i) {
    string key = to_string(i);
    string payload = "Exactly-once message #" + to_string(i);

    rd_kafka_resp_err_t produce_err =
        rd_kafka_producev(producer, RD_KAFKA_V_TOPIC(topic_name.c_str()),
                          RD_KAFKA_V_KEY(key.c_str(), key.size()),
                          RD_KAFKA_V_VALUE(payload.data(), payload.size()),
                          RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                          RD_KAFKA_V_END);

    if (produce_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
      cerr << "Failed to queue message " << i << ": "
           << rd_kafka_err2str(produce_err) << endl;
      produce_failed = true;
      break;
    }

    cout << "Queued message " << i << " in transaction." << endl;
    rd_kafka_poll(producer, 0);
  }

  if (!produce_failed) {
    rd_kafka_flush(producer, 10000);
    cout << "Committing transaction..." << endl;
    error = rd_kafka_commit_transaction(producer, 10000);
    if (error == nullptr) {
      cout << "Transaction committed successfully." << endl;
    } else {
      transaction_failed = true;
    }
  }

  if (produce_failed || error != nullptr) {
    transaction_failed = true;
    if (error != nullptr) {
      cerr << "commit_transaction failed: " << rd_kafka_error_string(error)
           << endl;
    }

    if (error == nullptr || rd_kafka_error_txn_requires_abort(error)) {
      cout << "Aborting transaction..." << endl;
      rd_kafka_error_t *abort_error = rd_kafka_abort_transaction(producer, 10000);
      if (abort_error != nullptr) {
        cerr << "abort_transaction failed: " << rd_kafka_error_string(abort_error)
             << endl;
        destroy_error(abort_error);
      } else {
        cout << "Transaction aborted." << endl;
      }
    }
  }

  destroy_error(error);
  rd_kafka_destroy(producer);
  return transaction_failed ? 1 : 0;
}
