from kafka import KafkaConsumer
import json

# Port should be 9093 with ENTRYPOINT (EXTERNAL)
# and 9092 with CMD
consumer = KafkaConsumer(
    "my_test_topic",
    bootstrap_servers="localhost:9093",
    auto_offset_reset="earliest",
    enable_auto_commit=True,
    value_deserializer=lambda x: json.loads(x.decode("utf-8")),
)

if __name__ == "__main__":
    for message in consumer:
        print(message.value)
