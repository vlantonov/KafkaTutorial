# Demo

## Video
* [Kafka Crash Course - Hands-On Project](https://www.youtube.com/watch?v=B7CwU_tNYIE)

## Virtual environment
* `python3 -m venv kafkademo`
* `source kafkademo/bin/activate`
* `deactivate`

## Install confluent-kafka dependency
* `pip3 install confluent-kafka`
* confluent-kafka-2.12.2

## Validate that the topic was created in kafka container
* `docker exec -it kafka kafka-topics --list --bootstrap-server localhost:9092`

## Describe that topic and see its partitions
* `docker exec -it kafka kafka-topics --bootstrap-server localhost:9092 --describe --topic orders`

## View all events in a topic
* `docker exec -it kafka kafka-console-consumer --bootstrap-server localhost:9092 --topic orders --from-beginning`

## Git Repo
* <https://gitlab.com/twn-youtube/kafka-crash-course>

## Official Docs
* [Introduction to Apache Kafka](https://docs.confluent.io/kafka/introduction.html)
* [Kafka Command-Line Interface (CLI) Tools](https://docs.confluent.io/kafka/operations-tools/kafka-tools.html)

## PRE-REQUISITES
*  [Python Full Course for Beginners](https://www.youtube.com/watch?v=t8pPdKYpowI)
*  [Docker in 1 Hour](https://www.youtube.com/watch?v=pg19Z8LL06w)
*  [Docker Compose Tutorial](https://www.youtube.com/watch?v=SXwC9fSwct8)
*  [Kafka explained](https://www.youtube.com/watch?v=QkdkLdMBuL0)
