"""Event generator for the stream-processing demo.

Produces JSON events to the ``events`` topic and deliberately re-sends some
previously used ids (controlled by ``DUP_RATE``) so the worker's dedup logic
has real duplicates to drop. Runs fast relative to the worker's per-event
delay to make backpressure observable.
"""

import json
import os
import random
import time

from confluent_kafka import Producer

BROKER = os.environ.get("KAFKA_BOOTSTRAP", "kafka:9092")
NUM_EVENTS = int(os.environ.get("NUM_EVENTS", "60"))
DUP_RATE = float(os.environ.get("DUP_RATE", "0.25"))
PRODUCE_DELAY = float(os.environ.get("PRODUCE_DELAY", "0.05"))
STARTUP_DELAY = float(os.environ.get("STARTUP_DELAY", "15"))

USERS = ["alice", "bob", "carol", "dave"]


def main() -> None:
    # Give the worker time to start and declare its topics.
    print(f"producer: waiting {STARTUP_DELAY}s for the stream app to start...", flush=True)
    time.sleep(STARTUP_DELAY)

    producer = Producer({"bootstrap.servers": BROKER, "acks": "all"})
    produced_ids: list[str] = []

    for i in range(NUM_EVENTS):
        if produced_ids and random.random() < DUP_RATE:
            event_id = random.choice(produced_ids)  # replay -> duplicate
            tag = "DUP"
        else:
            event_id = f"evt-{i:04d}"
            produced_ids.append(event_id)
            tag = "NEW"

        event = {
            "id": event_id,
            "user": random.choice(USERS),
            "amount": round(random.uniform(1, 500), 2),
            "timestamp": time.time(),
        }
        producer.produce("events", value=json.dumps(event).encode("utf-8"))
        producer.poll(0)
        print(f"produced [{tag}] {event}", flush=True)
        time.sleep(PRODUCE_DELAY)

    producer.flush()
    print(f"producer: done, sent {NUM_EVENTS} events", flush=True)


if __name__ == "__main__":
    main()
