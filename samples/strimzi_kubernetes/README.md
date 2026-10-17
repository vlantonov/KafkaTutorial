# Strimzi on Kubernetes (Kind) Demo

This demo runs Apache Kafka on Kubernetes using the Strimzi operator.
It uses Strimzi `v1` CRDs (KRaft + NodePools).

## Prerequisites

- [Docker](https://docs.docker.com/get-docker/)
- [kind](https://kind.sigs.k8s.io/)
- [kubectl](https://kubernetes.io/docs/tasks/tools/)

## 1) Create local Kubernetes cluster

```bash
kind create cluster --name kafka-strimzi --config ./kind-config.yaml
kubectl cluster-info --context kind-kafka-strimzi
```

### Minikube alternative

```bash
minikube -p kafka-strimzi-demo start --driver=docker --cpus=2 --memory=4096
kubectl config use-context kafka-strimzi-demo
```

## 2) Install Strimzi Cluster Operator

```bash
kubectl create namespace kafka
kubectl apply -f https://strimzi.io/install/latest?namespace=kafka -n kafka
kubectl -n kafka wait deployment/strimzi-cluster-operator --for=condition=Available=True --timeout=180s
```

## 3) Deploy Kafka cluster + topic + toolbox pod

```bash
kubectl apply -f ./manifests/01-kafka-cluster.yaml
kubectl apply -f ./manifests/02-kafka-topic.yaml
kubectl apply -f ./manifests/03-kafka-client.yaml
```

Wait until Kafka is ready:

```bash
kubectl -n kafka wait kafka/my-cluster --for=condition=Ready=True --timeout=300s
kubectl -n kafka get pods
```

## 4) Produce and consume messages

Open consumer terminal:

```bash
kubectl -n kafka exec -it kafka-client -- \
  bin/kafka-console-consumer.sh \
  --bootstrap-server my-cluster-kafka-bootstrap:9092 \
  --topic demo-topic \
  --from-beginning
```

Open producer terminal:

```bash
kubectl -n kafka exec -it kafka-client -- \
  bin/kafka-console-producer.sh \
  --bootstrap-server my-cluster-kafka-bootstrap:9092 \
  --topic demo-topic
```

Type a few messages in the producer; they should appear in the consumer.

## 5) Verify Strimzi resources

```bash
kubectl -n kafka get kafka
kubectl -n kafka get kafkatopic
kubectl -n kafka get svc my-cluster-kafka-bootstrap
```

## Cleanup

```bash
kind delete cluster --name kafka-strimzi
```
