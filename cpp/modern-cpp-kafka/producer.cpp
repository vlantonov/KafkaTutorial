#include <kafka/KafkaProducer.h>

#include <chrono>
#include <iostream>
#include <string>

using namespace std;

using namespace kafka;
using namespace kafka::clients::producer;

int main(int argc, char* argv[]) {
  // Kafka broker address
  string brokers = "localhost:9092";
  string topic_name = "test-topic";

  if (argc > 1) {
    brokers = argv[1];
  }
  if (argc > 2) {
    topic_name = argv[2];
  }

  // Configure the producer
  Properties props({
      {"bootstrap.servers", brokers},
      //{"client.id", "modern-cpp-kafka-producer"}
  });

  try {
    // Create producer instance
    KafkaProducer producer(props);

    cout << "Kafka Producer started. Broker: " << brokers << endl;
    cout << "Topic: " << topic_name << endl;
    cout << "Type messages (Ctrl+C to exit):" << endl;

    int msg_count = 0;
    string line;

    while (getline(cin, line)) {
      if (line.empty()) continue;

      // Create producer record
      string key = to_string(msg_count);
      ProducerRecord record(topic_name, Key(key.c_str(), key.size()),
                            Value(line.c_str(), line.size()));

      // Send message asynchronously with callback
      producer.send(record, [msg_count, line](const RecordMetadata& metadata,
                                              const Error& error) {
        if (error) {
          cerr << "Failed to send message #" << msg_count << ": "
               << error.message() << endl;
        } else {
          cout << "Sent message #" << msg_count << ": " << line << endl;
          cout << "  Partition: "
               << metadata.partition()
               /*<< ", Offset: " << metadata.offset()*/
               << endl;
        }
      });

      msg_count++;
    }

    // Wait for all messages to be delivered
    cout << "\nFlushing remaining messages..." << endl;
    producer.flush(chrono::seconds(10));

    cout << "Total messages sent: " << msg_count << endl;

  } catch (const KafkaException& e) {
    cerr << "Kafka Error: " << e.what() << endl;
    return 1;
  }

  return 0;
}
