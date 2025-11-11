#include <cppkafka/cppkafka.h>

#include <chrono>
#include <csignal>
#include <iostream>
#include <string>

using namespace std;
using namespace cppkafka;

// Signal handler for graceful shutdown
bool running = true;
void signal_handler(int signal) { running = false; }

int main(int argc, char* argv[]) {
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

  // Set up signal handler
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Configure the consumer
  Configuration config = {
      {"metadata.broker.list", brokers},
      {"group.id", group_id},
      {"enable.auto.commit", true},
      {"auto.offset.reset", "earliest"},  // Start from beginning if no offset
      {"client.id", "cppkafka-consumer"}};

  try {
    // Create consumer instance
    Consumer consumer(config);

    // Subscribe to topic
    consumer.subscribe({topic_name});

    cout << "Kafka Consumer started" << endl;
    cout << "Broker: " << brokers << endl;
    cout << "Topic: " << topic_name << endl;
    cout << "Group ID: " << group_id << endl;
    cout << "Waiting for messages (Ctrl+C to exit)..." << endl << endl;

    int msg_count = 0;

    // Poll for messages
    while (running) {
      Message msg = consumer.poll(chrono::milliseconds(1000));

      if (msg) {
        // Check for errors
        if (msg.get_error()) {
          if (!msg.is_eof()) {
            cerr << "Error: " << msg.get_error() << endl;
          }
        } else {
          // Process message
          cout << "Received message #" << msg_count++ << endl;
          cout << "  Topic: " << msg.get_topic() << endl;
          cout << "  Partition: " << msg.get_partition() << endl;
          cout << "  Offset: " << msg.get_offset() << endl;

          if (msg.get_key()) {
            cout << "  Key: " << msg.get_key() << endl;
          }

          cout << "  Payload: " << msg.get_payload() << endl;
          cout << endl;
        }
      }
    }

    cout << "\nShutting down. Total messages received: " << msg_count << endl;

  } catch (const Exception& e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }

  return 0;
}
