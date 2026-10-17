import json
from datetime import datetime

from confluent_kafka import Consumer, KafkaError, KafkaException


def fmt_partition(tp):
    return f"{tp.topic}[{tp.partition}]"


def on_assign(consumer, partitions):
    names = ", ".join(fmt_partition(p) for p in partitions)
    print(
        f"[{datetime.now().strftime('%H:%M:%S')}] ASSIGNED {len(partitions)} partition(s): {names}"
    )
    consumer.assign(partitions)


def on_revoke(consumer, partitions):
    names = ", ".join(fmt_partition(p) for p in partitions)
    print(
        f"[{datetime.now().strftime('%H:%M:%S')}] REVOKED {len(partitions)} partition(s): {names}"
    )
    consumer.unassign()


def on_lost(consumer, partitions):
    names = ", ".join(fmt_partition(p) for p in partitions)
    print(f"[{datetime.now().strftime('%H:%M:%S')}] LOST {len(partitions)} partition(s): {names}")
    consumer.unassign()


def main():
    topic = "my_test_topic"
    group_id = "rebalance-demo-group"
    consumer = Consumer(
        {
            "bootstrap.servers": "localhost:9093",
            "group.id": group_id,
            "auto.offset.reset": "earliest",
            "enable.auto.commit": True,
        }
    )

    print(f"Starting rebalance demo consumer for topic '{topic}' with group '{group_id}'")
    print("Start a second instance to trigger reassignment. Press Ctrl+C to stop.")

    consumer.subscribe([topic], on_assign=on_assign, on_revoke=on_revoke, on_lost=on_lost)

    try:
        while True:
            msg = consumer.poll(1.0)
            if msg is None:
                continue
            if msg.error():
                if msg.error().code() == KafkaError._PARTITION_EOF:
                    continue
                raise KafkaException(msg.error())

            payload = msg.value()
            try:
                decoded = json.loads(payload.decode("utf-8"))
            except (json.JSONDecodeError, UnicodeDecodeError, AttributeError):
                decoded = payload.decode("utf-8") if isinstance(payload, bytes) else str(payload)

            print(
                f"[{datetime.now().strftime('%H:%M:%S')}] "
                f"CONSUMED {msg.topic()}[{msg.partition()}]@{msg.offset()} key={msg.key()} value={decoded}"
            )
    except KeyboardInterrupt:
        print("Stopping rebalance demo consumer...")
    finally:
        consumer.close()


if __name__ == "__main__":
    main()
