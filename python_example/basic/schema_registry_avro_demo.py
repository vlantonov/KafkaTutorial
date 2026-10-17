import io
import json
from datetime import datetime

from confluent_kafka import Consumer, Producer
from confluent_kafka.schema_registry import Schema, SchemaRegistryClient
from fastavro import parse_schema, schemaless_reader, schemaless_writer


TOPIC = "avro-user-events"
SUBJECT = f"{TOPIC}-value"
SCHEMA_STR = json.dumps(
    {
        "type": "record",
        "name": "UserEvent",
        "namespace": "tutorial",
        "fields": [
            {"name": "user_id", "type": "string"},
            {"name": "action", "type": "string"},
            {"name": "event_ts", "type": "long"},
        ],
    }
)


def encode_confluent_avro(schema_id: int, payload: dict) -> bytes:
    record_schema = parse_schema(json.loads(SCHEMA_STR))
    out = io.BytesIO()
    out.write(b"\x00")
    out.write(schema_id.to_bytes(4, byteorder="big", signed=False))
    schemaless_writer(out, record_schema, payload)
    return out.getvalue()


def decode_confluent_avro(message_value: bytes) -> tuple[int, dict]:
    if len(message_value) < 5 or message_value[0] != 0:
        raise ValueError("Invalid Confluent Avro wire format payload")

    schema_id = int.from_bytes(message_value[1:5], byteorder="big", signed=False)
    record_schema = parse_schema(json.loads(SCHEMA_STR))
    decoded = schemaless_reader(io.BytesIO(message_value[5:]), record_schema)
    return schema_id, decoded


def main():
    registry = SchemaRegistryClient({"url": "http://localhost:8081"})
    schema_id = registry.register_schema(SUBJECT, Schema(SCHEMA_STR, "AVRO"))
    print(f"Registered/loaded Avro schema id={schema_id} subject={SUBJECT}")

    producer = Producer({"bootstrap.servers": "localhost:9093"})
    event = {
        "user_id": "u-42",
        "action": "checkout",
        "event_ts": int(datetime.now().timestamp()),
    }
    encoded = encode_confluent_avro(schema_id, event)
    producer.produce(TOPIC, key=event["user_id"], value=encoded)
    producer.flush(10)
    print(f"Produced Avro event to '{TOPIC}': {event}")

    consumer = Consumer(
        {
            "bootstrap.servers": "localhost:9093",
            "group.id": "avro-demo-group",
            "auto.offset.reset": "earliest",
            "enable.auto.commit": False,
        }
    )
    consumer.subscribe([TOPIC])
    print("Consuming one Avro event...")

    msg = consumer.poll(10.0)
    if msg is None:
        consumer.close()
        raise RuntimeError("Timed out waiting for Avro message")
    if msg.error():
        consumer.close()
        raise RuntimeError(f"Consumer error: {msg.error()}")

    read_schema_id, decoded = decode_confluent_avro(msg.value())
    print(f"Consumed Avro event schema_id={read_schema_id}: {decoded}")
    consumer.close()


if __name__ == "__main__":
    main()
