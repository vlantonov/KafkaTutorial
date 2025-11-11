# C++ cppkafka Project

This project provides a practical example of integrating Apache Kafka with C++ applications. It demonstrates:
- Message production and consumption
- Consumer group management
- Offset handling
- Error management
- Graceful shutdown patterns

## Features

### Producer Features
-  Interactive stdin-based message input
-  Configurable broker and topic
-  Message keys for partitioning
-  Synchronous delivery with flush
-  Message counter and acknowledgment
-  Error handling and reporting

### Consumer Features
-  Topic subscription with consumer groups
-  Automatic offset management
-  Message metadata display (partition, offset, key)
-  Graceful shutdown with signal handling
-  Configurable starting position (earliest/latest)
-  Real-time message processing


## Prerequisites

- [CMake](https://cmake.org/) (>= 3.21)
- [Conan](https://conan.io/) (>= 2.0)
- [Docker](https://www.docker.com/) & [Docker Compose](https://docs.docker.com/compose/)

## Getting Started

Using `build` folder

### Debug build
```sh
conan install .. --build=missing -pr:b=default -s build_type=Debug
cmake .. --preset conan-debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd Debug
make
```
* Use `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to export list of compile commands
* For cmake<3.23 :
```sh
cd Debug
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
```

### Release build
```sh
conan install .. --build=missing -pr:b=default -s build_type=Release
cmake .. --preset conan-release
cd Release
make
```

## 📖 Usage

### Producer

The producer application reads messages from standard input and publishes them to Kafka.

**Basic Usage:**
```bash
./cppkafka_example_producer
```

**With Custom Configuration:**
```bash
./cppkafka_example_producer <broker-address> <topic-name>

# Example
./cppkafka_example_producer localhost:9092 my-custom-topic
```

**Interactive Session Example:**
```bash
$ ./cppkafka_example_producer
Kafka Producer started. Broker: localhost:9092
Topic: test-topic
Type messages (Ctrl+C to exit):
Hello, Kafka!
Sent message #0: Hello, Kafka!
This is my second message
Sent message #1: This is my second message
Testing the producer
Sent message #2: Testing the producer
^C
Total messages sent: 3
```

### Consumer

The consumer application subscribes to a Kafka topic and displays received messages in real-time.

**Basic Usage:**
```bash
./cppkafka_example_consumer
```

**With Custom Configuration:**
```bash
./cppkafka_example_consumer <broker-address> <topic-name> <consumer-group-id>

# Example
./cppkafka_example_consumer localhost:9092 my-custom-topic my-group
```

**Output Example:**
```bash
$ ./cppkafka_example_consumer
Kafka Consumer started
Broker: localhost:9092
Topic: test-topic
Group ID: test-consumer-group
Waiting for messages (Ctrl+C to exit)...

Received message #0
  Topic: test-topic
  Partition: 0
  Offset: 0
  Key: 0
  Payload: Hello, Kafka!

Received message #1
  Topic: test-topic
  Partition: 0
  Offset: 1
  Key: 1
  Payload: This is my second message

^C
Shutting down. Total messages received: 2
```

### Running Producer and Consumer Together

**Terminal 1 (Consumer):**
```bash
cd build
./cppkafka_example_consumer
```

**Terminal 2 (Producer):**
```bash
cd build
./cppkafka_example_producer
# Type messages and press Enter
```

Messages typed in the producer terminal will appear in real-time in the consumer terminal.

## ⚙️ Configuration

### Producer Configuration Options

Edit the `Configuration` object in `producer.cpp`:

```cpp
Configuration config = {
    { "metadata.broker.list", brokers },
    { "client.id", "cppkafka-producer" },
    // Additional options:
    { "compression.codec", "snappy" },        // Compression: none, gzip, snappy, lz4, zstd
    { "acks", "all" },                        // Acknowledgment level: 0, 1, all
    { "retries", "3" },                       // Number of retries
    { "batch.size", "16384" },                // Batch size in bytes
    { "linger.ms", "10" },                    // Wait time for batching
    { "max.in.flight.requests.per.connection", "5" }
};
```

### Consumer Configuration Options

Edit the `Configuration` object in `consumer.cpp`:

```cpp
Configuration config = {
    { "metadata.broker.list", brokers },
    { "group.id", group_id },
    { "enable.auto.commit", true },
    { "auto.offset.reset", "earliest" },      // earliest, latest, none
    { "client.id", "cppkafka-consumer" },
    // Additional options:
    { "session.timeout.ms", "10000" },        // Session timeout
    { "max.poll.interval.ms", "300000" },     // Max poll interval
    { "fetch.min.bytes", "1" },               // Minimum fetch size
    { "fetch.max.wait.ms", "500" },           // Max wait for fetch
    { "auto.commit.interval.ms", "5000" }     // Auto-commit interval
};
```

### Common Configuration Parameters

| Parameter | Description | Values |
|-----------|-------------|--------|
| `metadata.broker.list` | Kafka broker addresses | `host:port` (comma-separated for multiple) |
| `group.id` | Consumer group ID | Any string |
| `auto.offset.reset` | Starting position for new consumers | `earliest`, `latest`, `none` |
| `enable.auto.commit` | Automatic offset commits | `true`, `false` |
| `compression.codec` | Message compression | `none`, `gzip`, `snappy`, `lz4`, `zstd` |
| `acks` | Producer acknowledgment | `0`, `1`, `all` |

## Additional Resources

- [Apache Kafka Documentation](https://kafka.apache.org/documentation/)
- [cppkafka GitHub Repository](https://github.com/mfontanini/cppkafka)
- [librdkafka Documentation](https://docs.confluent.io/platform/current/clients/librdkafka/html/index.html)
- [Kafka Best Practices](https://kafka.apache.org/documentation/#design)