# Kafka C example

## Docker images
Used instead of `latest`:
* `confluentinc/cp-zookeeper:7.3.2`
* `confluentinc/cp-kafka:7.3.2`

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

   `kafka-topics --create --topic purchases --bootstrap-server localhost:9092`

## Start Kafka producer

   `./KafkaProducer getting-started.ini`

## Start Kafka consumer
In new terminal window

   `./KafkaConsumer getting-started.ini`

## Close Kafka containers

   `docker-compose down`

## Links
* [Getting Started with Apache Kafka and C/C++](https://developer.confluent.io/get-started/c/)
* [Starting with Apache Kafka](https://medium.com/@danielchristofolli/starting-with-apache-kafka-ae20669a8160)
* <https://github.com/dchristofolli/kafka-sample/blob/main/docker-compose.yml>
* [Fresh install Docker cgroup mountpoint does not exist](https://forum.garudalinux.org/t/fresh-install-docker-cgroup-mountpoint-does-not-exist/1684/2)

