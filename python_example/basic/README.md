# Kafka Sample

## Temp fix
Problem: Docker 20 needed.

```
sudo mkdir /sys/fs/cgroup/systemd
sudo mount -t cgroup -o none,name=systemd cgroup /sys/fs/cgroup/systemd
```

Add this to your `/etc/fstab`
```
cgroup /sys/fs/cgroup cgroup defaults
```

## Fix
[How To Install and Use Docker on Ubuntu 20.04](https://www.digitalocean.com/community/tutorials/how-to-install-and-use-docker-on-ubuntu-20-04)
```
sudo apt update
sudo apt install apt-transport-https ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu focal stable"
apt-cache policy docker-ce
sudo apt install docker-ce
sudo systemctl status docker
```

## Start
   `docker-compose up -d`

Check if it is running

   `docker-compose ps`

Get `{docker-id}` for Kafka container

   `docker ps`

Enter the running container

   `docker exec -it {docker-id} /bin/bash`

## Create Kafka topic
   `kafka-topics --create --topic my_test_topic --bootstrap-server localhost:9092`

## Install Python requirements
   `pip install -r requirements.txt`

## Install using Docker

   `docker build -t python-kafka .`

## Start Kafka producer

With CMD

   `docker run --network=host python-kafka`

With ENTRYPOINT

   `docker run --network=host python-kafka ./producer.py`

## Start exactly-once transactional producer

This example uses `confluent-kafka` with Kafka transactions (`transactional.id` + idempotence) to produce a committed batch atomically.

With ENTRYPOINT

   `docker run --network=host python-kafka ./transactional_producer.py`

## Start Kafka consumer

   `docker run --network=host python-kafka ./consumer.py`

## Start consumer rebalance demo (partition reassignment logs)

Run two consumers with the same group to observe `ASSIGNED` / `REVOKED` logs when partitions are moved between members.

Terminal 1:

   `docker run --network=host python-kafka ./rebalance_consumer.py`

Terminal 2:

   `docker run --network=host python-kafka ./rebalance_consumer.py`

Example output:

```
[00:12:10] ASSIGNED 1 partition(s): my_test_topic[0]
[00:12:16] REVOKED 1 partition(s): my_test_topic[0]
[00:12:16] ASSIGNED 1 partition(s): my_test_topic[0]
```

## Schema Registry + Avro / Protobuf demo

Start Schema Registry (expects Kafka broker on `localhost:9093`):

`docker run --rm --network=host -p 8081:8081 -e SCHEMA_REGISTRY_KAFKASTORE_BOOTSTRAP_SERVERS=PLAINTEXT://localhost:9093 -e SCHEMA_REGISTRY_HOST_NAME=localhost -e SCHEMA_REGISTRY_LISTENERS=http://0.0.0.0:8081 confluentinc/cp-schema-registry:7.7.0`

Run Avro demo (registers schema, produces, then consumes one record):

`docker run --network=host python-kafka ./schema_registry_avro_demo.py`

Run Protobuf demo (registers schema, produces, then consumes one record):

`docker run --network=host python-kafka ./schema_registry_protobuf_demo.py`

Expected logs include schema IDs from Schema Registry:

```
Registered/loaded Avro schema id=1 subject=avro-user-events-value
Produced Avro event to 'avro-user-events': {'user_id': 'u-42', ...}
Consumed Avro event schema_id=1: {'user_id': 'u-42', ...}
Registered/loaded Protobuf schema id=2 subject=proto-user-events-value
Produced Protobuf event to 'proto-user-events': {'user_id': 'u-84', ...}
Consumed Protobuf event schema_id=2: {'user_id': 'u-84', ...}
```

Can be checked by

   `kafka-console-consumer --topic my_test_topic --bootstrap-server localhost:9092`

## Close Kafka containers

`docker-compose down`

## Links
* [Setting up Kafka on Docker for Local Development](https://hackernoon.com/setting-up-kafka-on-docker-for-local-development)
* <https://github.com/choyiny/kafka-on-docker-development>
* [Fresh install Docker cgroup mountpoint does not exist](https://forum.garudalinux.org/t/fresh-install-docker-cgroup-mountpoint-does-not-exist/1684/2)
* [How to “Dockerize” Your Python Applications](https://www.docker.com/blog/how-to-dockerize-your-python-applications/)