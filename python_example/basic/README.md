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

## Start Kafka consumer

   `docker run --network=host python-kafka ./consumer.py`

Can be checked by

   `kafka-console-consumer --topic my_test_topic --bootstrap-server localhost:9092`

## Close Kafka containers

`docker-compose down`

## Links
* [Setting up Kafka on Docker for Local Development](https://hackernoon.com/setting-up-kafka-on-docker-for-local-development)
* <https://github.com/choyiny/kafka-on-docker-development>
* [Fresh install Docker cgroup mountpoint does not exist](https://forum.garudalinux.org/t/fresh-install-docker-cgroup-mountpoint-does-not-exist/1684/2)
* [How to “Dockerize” Your Python Applications](https://www.docker.com/blog/how-to-dockerize-your-python-applications/)