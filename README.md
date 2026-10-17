# KafkaTutorial

Scenario-driven Kafka walkthrough: **Producer → Broker → Consumer**.

This README gives one fast, practical flow you can run locally and observe with real logs.

---

## Architecture (what you are about to run)

1. **Producer** sends JSON events to Kafka topic `my_test_topic`
2. **Broker** stores/replicates events and serves them to consumers
3. **Consumer** reads events and prints them

```text
[Python Producer] ---> [Kafka Broker] ---> [Python Consumer]
```

We use the runnable setup in [python_example/basic/](./python_example/basic/).

---

## Prerequisites

- Docker + Docker Compose
- Python 3 (optional if running inside Docker only)

---

## Step 1 — Start broker infrastructure

```bash
cd python_example/basic
docker compose up -d
docker compose ps
```

You should see `kafka` and `zookeeper` containers in `Up` state.

### Screenshot checkpoint

- Capture `docker compose ps` output showing healthy services.

---

## Step 2 — Create topic on broker

```bash
docker ps
# Copy Kafka container id, then:
docker exec -it <kafka-container-id> kafka-topics --create \
  --topic my_test_topic \
  --bootstrap-server localhost:9092
```

Verify:

```bash
docker exec -it <kafka-container-id> kafka-topics --list \
  --bootstrap-server localhost:9092
```

Expected to include `my_test_topic`.

### Screenshot checkpoint

- Capture `kafka-topics --list` showing `my_test_topic`.

---

## Step 3 — Run consumer (downstream)

Terminal A:

```bash
docker build -t python-kafka .
docker run --network=host python-kafka ./consumer.py
```

Consumer waits for messages.

### Screenshot checkpoint

- Capture consumer terminal before messages (idle waiting state).

---

## Step 4 — Run producer (upstream)

Terminal B:

```bash
docker run --network=host python-kafka ./producer.py
```

Producer sends one JSON event.

---

## Step 5 — Observe end-to-end logs

You should now see the message printed by the consumer.

### Example producer-side behavior

```text
# producer exits successfully after future.get(timeout=1)
```

### Example consumer-side log

```text
{'author': 'author_name', 'content': 'Hello World!', 'created_at': '2026-10-18T01:00:00'}
```

### Screenshot checkpoint

- Capture both terminals:
  - Producer run command and successful exit
  - Consumer showing the received JSON payload

---

## Optional scenario extensions

- **Exactly-once transactions**: run [transactional_producer.py](./python_example/basic/transactional_producer.py)
- **Consumer rebalance**: run two instances of [rebalance_consumer.py](./python_example/basic/rebalance_consumer.py)
- **Schema Registry + Avro/Protobuf**:
  - [schema_registry_avro_demo.py](./python_example/basic/schema_registry_avro_demo.py)
  - [schema_registry_protobuf_demo.py](./python_example/basic/schema_registry_protobuf_demo.py)
- **Kubernetes / Strimzi**: [samples/strimzi_kubernetes/README.md](./samples/strimzi_kubernetes/README.md)

---

## Client comparison (C / C++ / Python)

| Dimension | C (`librdkafka`) | C++ (`librdkafka` wrappers) | Python (`kafka-python`, `confluent-kafka`) |
|---|---|---|---|
| Configuration | Low-level and explicit | Similar config keys with safer API ergonomics | Fast dict-based setup |
| Delivery semantics | Full support: idempotence + transactions | Same semantics as C (via librdkafka) | Full EOS with `confluent-kafka`; simpler path with `kafka-python` |
| Throughput profile | Highest ceiling | Near-C in most service workloads | Lower raw ceiling, fastest developer iteration |

---

## Cleanup

```bash
docker compose down
```