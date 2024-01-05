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

## Start
   `docker-compose up -d`

Check if it is running

   `docker-compose ps`

Get `{docker-id}` for Kafka container

   `docker ps`

Enter the running container

   `docker exec -it {docker-id} /bin/bash`

## Create Kafka topic
   `kafka-topics --create --topic my_test_topic --partitions 3 --replication-factor 1 --bootstrap-server localhost:9092`

## Create Kafka producer
   `kafka-console-producer --topic my_test_topic --bootstrap-server localhost:9092`

## Create Kafka consumer
Open new terminal window

   `docker exec -it {docker-id} /bin/bash`

Create the consumer

   `kafka-console-consumer --topic my_test_topic --bootstrap-server localhost:9092`

## Close Kafka containers

`docker-compose down`

## Links
* [Setting up Kafka on Docker for Local Development](https://hackernoon.com/setting-up-kafka-on-docker-for-local-development)
* <https://github.com/choyiny/kafka-on-docker-development>
* [Fresh install Docker cgroup mountpoint does not exist](https://forum.garudalinux.org/t/fresh-install-docker-cgroup-mountpoint-does-not-exist/1684/2)