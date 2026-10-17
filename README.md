# KafkaTutorial
Kafka Tutorial

## Samples

* [Strimzi on Kubernetes demo](./samples/strimzi_kubernetes/README.md)

## Client comparison (C / C++ / Python)

| Dimension | C (`librdkafka`) | C++ (`librdkafka` wrappers) | Python (`kafka-python`, `confluent-kafka`) |
|---|---|---|---|
| Configuration style | INI/properties style and low-level config keys (very explicit) | Mostly same Kafka/librdkafka keys with C++ API ergonomics | Dict-based config in code; quickest to iterate |
| Delivery semantics | At-most-once, at-least-once, idempotent, and transactions (exactly-once) are fully supported | Same semantics as C when backed by librdkafka; easier callback and lifecycle handling | `confluent-kafka` supports idempotence + transactions; `kafka-python` is simpler but less feature-complete for EOS |
| Rebalance control | Full control via callbacks and explicit assign/unassign | Full control with cleaner callback code | Full control in `confluent-kafka` (`on_assign` / `on_revoke`); easy to prototype |
| Schema Registry / Avro / Protobuf | Possible, but usually requires manual wire-format handling or extra glue | Same as C for low-level producers unless a higher-level serialization layer is added | Most convenient: direct Schema Registry client usage and fast Avro/Protobuf demos |
| Throughput profile (practical) | Highest throughput / lowest overhead for tuned workloads | Near-C throughput in practice (depending on wrapper and code path) | Lower raw throughput than C/C++ in pure Python; `confluent-kafka` narrows the gap significantly |
| Best fit | Latency-critical services, embedded/system-level components | Production services needing both performance and safer abstractions | Fast development, scripting, data workflows, and feature demos |

**Rule of thumb:** start with Python for fast iteration, move to C++ for long-lived high-throughput services, and use C when you need maximum low-level control.

## Links
* [Apache Kafka Quickstart](https://kafka.apache.org/quickstart)
* [What Is Apache Kafka?](https://developer.confluent.io/what-is-apache-kafka/)
* [Getting Started with Apache Kafka and C/C++](https://developer.confluent.io/get-started/c/#prerequisites)
* [Apache Kafka Tutorial — Kafka For Beginners](https://medium.com/@patelharshali136/apache-kafka-tutorial-kafka-for-beginners-a58140cef84f)
* [Starting with Apache Kafka](https://medium.com/@danielchristofolli/starting-with-apache-kafka-ae20669a8160)
* [Steps to Install Kafka on Ubuntu 20.04](https://hevodata.com/blog/how-to-install-kafka-on-ubuntu/)
* [Setting up Kafka on Docker for Local Development](https://hackernoon.com/setting-up-kafka-on-docker-for-local-development)
* <https://github.com/dchristofolli/kafka-sample/blob/main/docker-compose.yml>
* [Monitoring Kafka in Kubernetes with Prometheus and Grafana](https://vkontech.com/monitoring-kafka-in-kubernetes-with-prometheus-and-grafana/)
* [Kafka Crash Course - Hands-On Project](https://www.youtube.com/watch?v=B7CwU_tNYIE)
* <github.com/ndymko/kafka-article-2>, <github.com/ndymko/kafka-article-3> , <github.com/ndymko/kafka-article-4>
* [Мониторинг Strimzi Kafka через kube-prometheus-stack](https://habr.com/ru/articles/992494/)