# librdkafka C++ Example Project

This project demonstrates how to use Apache Kafka with C++ using librdkafka directly - the high-performance C library that powers most Kafka clients.

This project provides a practical example of integrating Apache Kafka with C++ applications. It demonstrates:
- Message production and consumption
- Consumer group management
- Offset handling
- Error management
- Graceful shutdown patterns

## Why librdkafka?

librdkafka is the underlying C library used by most Kafka client implementations:
- **High Performance** - Optimized C implementation
- **Battle-tested** - Used in production by thousands of companies
- **Feature-complete** - Supports all Kafka features
- **Cross-platform** - Works on Linux, macOS, Windows
- **Low-level control** - Direct access to all Kafka capabilities

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
./rdkafka_producer
```

**With Custom Configuration:**
```bash
./rdkafka_producer <broker-address> <topic-name>

# Example
./rdkafka_producer localhost:9092 my-custom-topic
```

**Interactive Session Example:**
```bash
$ ./rdkafka_producer
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
./rdkafka_consumer
```

**With Custom Configuration:**
```bash
./rdkafka_consumer <broker-address> <topic-name> <consumer-group-id>

# Example
./rdkafka_consumer localhost:9092 my-custom-topic my-group
```

**Output Example:**
```bash
$ ./rdkafka_consumer
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
./rdkafka_consumer
```

**Terminal 2 (Producer):**
```bash
cd build
./rdkafka_producer
# Type messages and press Enter
```

Messages typed in the producer terminal will appear in real-time in the consumer terminal.

## Configuration Options

### Common Producer Settings:

```cpp
rd_kafka_conf_set(conf, "bootstrap.servers", "localhost:9092", ...);
rd_kafka_conf_set(conf, "compression.type", "snappy", ...);
rd_kafka_conf_set(conf, "acks", "all", ...);
rd_kafka_conf_set(conf, "retries", "3", ...);
rd_kafka_conf_set(conf, "linger.ms", "10", ...);
rd_kafka_conf_set(conf, "batch.size", "16384", ...);
```

### Common Consumer Settings:

```cpp
rd_kafka_conf_set(conf, "bootstrap.servers", "localhost:9092", ...);
rd_kafka_conf_set(conf, "group.id", "my-group", ...);
rd_kafka_conf_set(conf, "auto.offset.reset", "earliest", ...);  // or "latest"
rd_kafka_conf_set(conf, "enable.auto.commit", "true", ...);
rd_kafka_conf_set(conf, "session.timeout.ms", "10000", ...);
rd_kafka_conf_set(conf, "max.poll.interval.ms", "300000", ...);
```

## Advanced Features

### Manual Offset Management

```cpp
// Disable auto-commit
rd_kafka_conf_set(conf, "enable.auto.commit", "false", ...);

// Manual commit after processing
rd_kafka_commit(consumer, NULL, 0);  // Commit current position

// Or commit specific offset
rd_kafka_topic_partition_list_t *offsets = rd_kafka_topic_partition_list_new(1);
rd_kafka_topic_partition_list_add(offsets, "topic", 0)->offset = 100;
rd_kafka_commit(consumer, offsets, 0);
```

### Synchronous Producer

```cpp
// Wait for delivery after each message
rd_kafka_producev(producer, ...);
rd_kafka_flush(producer, 5000);  // Wait up to 5 seconds
```

### Error Handling

```cpp
rd_kafka_resp_err_t err = rd_kafka_subscribe(consumer, topics);
if (err) {
    fprintf(stderr, "Subscribe failed: %s\n", rd_kafka_err2str(err));
}
```

## Performance Tuning

### Producer Optimization:
- Increase `batch.size` for better throughput
- Set `linger.ms` to allow batching
- Use compression (`snappy` or `lz4`)
- Adjust `buffer.memory` for high-throughput scenarios

### Consumer Optimization:
- Increase `fetch.min.bytes` to reduce fetch requests
- Tune `max.poll.records` for batch size
- Adjust `session.timeout.ms` for rebalance behavior

### Debug Logging
```cpp
rd_kafka_conf_set(conf, "debug", "broker,topic,msg", errstr, sizeof(errstr));
```

## Use librdkafka directly when:
- Maximum performance is required
- You need access to all features
- Building cross-language bindings
- You're comfortable with C-style APIs

## Resources

- [librdkafka GitHub](https://github.com/confluentinc/librdkafka)
- [librdkafka Documentation](https://docs.confluent.io/platform/current/clients/librdkafka/html/index.html)
- [Configuration Reference](https://github.com/confluentinc/librdkafka/blob/master/CONFIGURATION.md)
- [Apache Kafka Documentation](https://kafka.apache.org/documentation/)