import json
import struct
from datetime import datetime
from io import BytesIO

from confluent_kafka import Consumer, Producer
from confluent_kafka.schema_registry import Schema, SchemaRegistryClient


TOPIC = "proto-user-events"
SUBJECT = f"{TOPIC}-value"
PROTO_SCHEMA = """
syntax = "proto3";
package tutorial;

message UserEvent {
  string user_id = 1;
  string action = 2;
  int64 event_ts = 3;
}
""".strip()


def encode_uvarint(value: int) -> bytes:
    if value < 0:
        raise ValueError("uvarint cannot encode negative numbers")
    out = bytearray()
    while value >= 0x80:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    out.append(value)
    return bytes(out)


def decode_uvarint(stream: BytesIO) -> int:
    shift = 0
    value = 0
    while True:
        byte = stream.read(1)
        if not byte:
            raise ValueError("Unexpected EOF while decoding uvarint")
        b = byte[0]
        value |= (b & 0x7F) << shift
        if (b & 0x80) == 0:
            return value
        shift += 7


def encode_string_field(field_number: int, value: str) -> bytes:
    encoded = value.encode("utf-8")
    key = (field_number << 3) | 2
    return encode_uvarint(key) + encode_uvarint(len(encoded)) + encoded


def encode_int64_field(field_number: int, value: int) -> bytes:
    if value < 0:
        raise ValueError("This demo expects non-negative int64")
    key = (field_number << 3) | 0
    return encode_uvarint(key) + encode_uvarint(value)


def encode_user_event(event: dict) -> bytes:
    return (
        encode_string_field(1, event["user_id"])
        + encode_string_field(2, event["action"])
        + encode_int64_field(3, event["event_ts"])
    )


def decode_user_event(payload: bytes) -> dict:
    stream = BytesIO(payload)
    result = {}

    while stream.tell() < len(payload):
        key = decode_uvarint(stream)
        field_no = key >> 3
        wire_type = key & 0x07

        if field_no in (1, 2) and wire_type == 2:
            length = decode_uvarint(stream)
            data = stream.read(length)
            if len(data) != length:
                raise ValueError("Unexpected EOF while decoding string field")
            result["user_id" if field_no == 1 else "action"] = data.decode("utf-8")
        elif field_no == 3 and wire_type == 0:
            result["event_ts"] = decode_uvarint(stream)
        else:
            raise ValueError(f"Unsupported field={field_no} wire_type={wire_type}")

    return result


def encode_confluent_protobuf(schema_id: int, payload: bytes) -> bytes:
    # Confluent Protobuf payload layout:
    # magic-byte(0) + schema-id(4-bytes BE) + message-indexes + protobuf-payload
    # For top-level first message (index [0]), message-indexes is a single 0 byte.
    return b"\x00" + struct.pack(">I", schema_id) + b"\x00" + payload


def decode_confluent_protobuf(message_value: bytes) -> tuple[int, dict]:
    if len(message_value) < 6 or message_value[0] != 0:
        raise ValueError("Invalid Confluent Protobuf wire format payload")

    schema_id = struct.unpack(">I", message_value[1:5])[0]
    if message_value[5] != 0:
        raise ValueError("This demo supports only top-level message index [0]")

    decoded = decode_user_event(message_value[6:])
    return schema_id, decoded


def main():
    registry = SchemaRegistryClient({"url": "http://localhost:8081"})
    schema_id = registry.register_schema(SUBJECT, Schema(PROTO_SCHEMA, "PROTOBUF"))
    print(f"Registered/loaded Protobuf schema id={schema_id} subject={SUBJECT}")

    producer = Producer({"bootstrap.servers": "localhost:9093"})
    event = {
        "user_id": "u-84",
        "action": "payment",
        "event_ts": int(datetime.now().timestamp()),
    }
    encoded = encode_confluent_protobuf(schema_id, encode_user_event(event))
    producer.produce(TOPIC, key=event["user_id"], value=encoded)
    producer.flush(10)
    print(f"Produced Protobuf event to '{TOPIC}': {event}")

    consumer = Consumer(
        {
            "bootstrap.servers": "localhost:9093",
            "group.id": "protobuf-demo-group",
            "auto.offset.reset": "earliest",
            "enable.auto.commit": False,
        }
    )
    consumer.subscribe([TOPIC])
    print("Consuming one Protobuf event...")

    msg = consumer.poll(10.0)
    if msg is None:
        consumer.close()
        raise RuntimeError("Timed out waiting for Protobuf message")
    if msg.error():
        consumer.close()
        raise RuntimeError(f"Consumer error: {msg.error()}")

    read_schema_id, decoded = decode_confluent_protobuf(msg.value())
    print(f"Consumed Protobuf event schema_id={read_schema_id}: {decoded}")
    consumer.close()


if __name__ == "__main__":
    main()
