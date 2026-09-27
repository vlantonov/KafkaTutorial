"""Faust stream-processing worker.

Demonstrates four stream-processing concerns on a single Kafka topic:

* Deduplication   - a Faust table remembers event ids and drops repeats.
* Windowing       - a tumbling window counts unique events per time bucket.
* Backpressure    - a bounded stream buffer + slow processing pauses fetching
                    instead of growing memory without bound.
* Exactly-once    - consume -> update tables -> produce runs in one Kafka
                    transaction (``processing_guarantee='exactly_once'``).

Faust is the Python equivalent of Kafka Streams: same "topology of agents and
state tables backed by changelog topics" model, expressed in asyncio.
"""

import asyncio
import os
from datetime import timedelta

import faust

BROKER = os.environ.get("KAFKA_BROKER", "kafka://kafka:9092")
PROCESS_DELAY = float(os.environ.get("PROCESS_DELAY", "0.25"))
WINDOW_SIZE = float(os.environ.get("WINDOW_SIZE", "10.0"))
BUFFER_MAXSIZE = int(os.environ.get("STREAM_BUFFER_MAXSIZE", "8"))

app = faust.App(
    "stream-pipeline",
    broker=BROKER,
    store="memory://",
    # Exactly-once: consume + table writes + produce are one atomic transaction.
    processing_guarantee="exactly_once",
    # Bounded in-memory buffer: when full, Faust pauses fetching -> backpressure.
    stream_buffer_maxsize=BUFFER_MAXSIZE,
    topic_partitions=1,
    consumer_auto_offset_reset="earliest",
)


class Event(faust.Record, serializer="json"):
    id: str
    user: str
    amount: float
    timestamp: float


events_topic = app.topic("events", value_type=Event)
processed_topic = app.topic("processed-events", value_type=Event)

# Dedup state: event id -> 1 if already processed. Backed by a changelog topic
# so the state survives restarts / rebalances.
seen_ids = app.Table("seen-ids", default=int, partitions=1)

# Tumbling-window counter: number of unique events processed per WINDOW_SIZE
# seconds. `relative_to_now()` buckets by wall-clock time.
windowed_counts = (
    app.Table("windowed-counts", default=int, partitions=1)
    .tumbling(WINDOW_SIZE, expires=timedelta(minutes=5))
    .relative_to_now()
)

stats = {"received": 0, "processed": 0, "duplicates": 0}


@app.agent(events_topic)
async def process(stream):
    """Dedup, count into the current window, and forward downstream."""
    async for event in stream:
        stats["received"] += 1

        if seen_ids[event.id]:
            stats["duplicates"] += 1
            app.log.info("DUPLICATE dropped id=%s", event.id)
            continue

        seen_ids[event.id] = 1
        # Simulated slow work. With a fast producer and a small buffer this is
        # what triggers backpressure: Faust stops fetching until we catch up.
        await asyncio.sleep(PROCESS_DELAY)

        windowed_counts["events"] += 1
        stats["processed"] += 1

        # Emitted inside the same transaction as the offset commit and the
        # table updates above -> exactly-once end to end.
        await processed_topic.send(value=event)
        app.log.info(
            "PROCESSED id=%s user=%s amount=%.2f window_count=%s",
            event.id,
            event.user,
            event.amount,
            windowed_counts["events"].now(),
        )


@app.timer(interval=WINDOW_SIZE)
async def report():
    """Log running totals and the current tumbling-window count."""
    app.log.info(
        "REPORT received=%d processed=%d duplicates=%d current_window_count=%s",
        stats["received"],
        stats["processed"],
        stats["duplicates"],
        windowed_counts["events"].now(),
    )
