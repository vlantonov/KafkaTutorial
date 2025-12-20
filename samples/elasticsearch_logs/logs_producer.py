from kafka import KafkaProducer
import json
from datetime import datetime

producer = KafkaProducer(
    bootstrap_servers=["localhost:9092"],
    value_serializer=lambda v: json.dumps(v).encode("utf-8"),
)

# Send sample logs
for i in range(10):
    log = {
        "message": f"Application log {i}",
        "level": "INFO",
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "service": "my-app",
    }
    producer.send("logs", value=log)
    print(f"Sent: {log}")

producer.flush()
producer.close()
