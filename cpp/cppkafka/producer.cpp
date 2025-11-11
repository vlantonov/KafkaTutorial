#include <cppkafka/cppkafka.h>

#include <csignal>
#include <iostream>
#include <string>

using namespace std;
using namespace cppkafka;

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
  Configuration config = {{"metadata.broker.list", brokers},
                          {"client.id", "cppkafka-producer"}};

  try {
    // Create producer instance
    Producer producer(config);

    cout << "Kafka Producer started. Broker: " << brokers << endl;
    cout << "Topic: " << topic_name << endl;
    cout << "Type messages (Ctrl+C to exit):" << endl;

    int msg_count = 0;
    string line;

    while (getline(cin, line)) {
      if (line.empty()) continue;

      // Create message
      MessageBuilder builder(topic_name);
      builder.partition(-1);  // Use Kafka's default partitioner
      std::string msg_key = to_string(msg_count);
      builder.key(msg_key);  // Optional key
      builder.payload(line);

      // Send message
      producer.produce(builder);

      cout << "Sent message #" << msg_count << ": " << line << endl;
      msg_count++;

      // Flush to ensure delivery
      producer.flush();
    }

    cout << "\nTotal messages sent: " << msg_count << endl;

  } catch (const Exception& e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }

  return 0;
}
