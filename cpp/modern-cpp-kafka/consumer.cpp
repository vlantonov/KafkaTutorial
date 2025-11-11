#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#include "kafka/KafkaConsumer.h"

using namespace std;

using namespace kafka;
using namespace kafka::clients::consumer;

// Signal handler for graceful shutdown
atomic<bool> running(true);
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
  Properties props({
      {"bootstrap.servers", brokers}, {"group.id", group_id},
      //{"enable.auto.commit", "true"},
      //{"auto.offset.reset", "earliest"}
      //{"client.id", "modern-cpp-kafka-consumer"}
  });

  try {
    // Create consumer instance
    KafkaConsumer consumer(props);

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
      auto records = consumer.poll(chrono::milliseconds(1000));

      for (const auto& record : records) {
        // Check for errors
        if (record.error()) {
          cerr << "Error: " << record.error().message() << endl;
          continue;
        }

        // Process message
        cout << "Received message #" << msg_count++ << endl;
        cout << "  Topic: " << record.topic() << endl;
        cout << "  Partition: " << record.partition() << endl;
        cout << "  Offset: " << record.offset() << endl;
        cout << "  Timestamp: " << record.timestamp().toString() << endl;

        if (record.key().size() > 0) {
          cout << "  Key: " << record.key().toString() << endl;
        }

        cout << "  Payload: " << record.value().toString() << endl;
        cout << endl;
      }
    }

    // Close consumer
    consumer.close();

    cout << "\nShutting down. Total messages received: " << msg_count << endl;

  } catch (const KafkaException& e) {
    cerr << "Kafka Error: " << e.what() << endl;
    return 1;
  }

  return 0;
}
