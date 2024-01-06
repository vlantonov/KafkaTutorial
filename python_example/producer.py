from datetime import datetime
from kafka import KafkaProducer
import json

# Port should be 9093 with ENTRYPOINT (EXTERNAL)
# and 9092 with CMD
producer = KafkaProducer(
    bootstrap_servers=["localhost:9093"],
    value_serializer=lambda v: json.dumps(v).encode("utf-8"),
)

if __name__ == "__main__":
    future = producer.send(
        "my_test_topic",
        {
            "author": "author_name",
            "content": "Hello World!",
            "created_at": datetime.now().strftime("%Y-%m-%dT%H:%M:%S"),
        },
    )
    future.get(timeout=1)
