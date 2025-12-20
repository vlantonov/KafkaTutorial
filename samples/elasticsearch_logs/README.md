# Streaming logs through Kafka in Elasticsearch for Kibana visualization

## Start
```
docker-compose up -d
```

### Stop when done
```
docker-compose down
```

## Ensure Kafka Topic Exists and Has Data

### Create the topic if it doesn't exist
```
docker exec -it kafka kafka-topics --create \
  --bootstrap-server localhost:9092 \
  --topic logs \
  --partitions 1 \
  --replication-factor 1
```

### List topics to verify
```
docker exec -it kafka kafka-topics --list \
  --bootstrap-server localhost:9092
```

### Send a test message
```
docker exec -it kafka kafka-console-producer \
  --bootstrap-server localhost:9092 \
  --topic logs
```
* Then type: `{"message": "test log", "level": "INFO", "timestamp": "2024-12-20T10:00:00Z"}`

## Configure Kibana Index Pattern
Once data is flowing, configure Kibana

### Access Kibana
* Open browser: `http://localhost:5601`

### Create Index Pattern
* Go to **Stack Management** (hamburger menu → Management → Stack Management)
* Click **Index Patterns** (under Kibana section)
* Click **Create index pattern**
* Enter index pattern: `kafka-logs-*`
* Click **Next step**
* Select **@timestamp** as the time field
* Click **Create index pattern**

### View Logs in Discover
* Go to **Discover** (hamburger menu → Analytics → Discover)
* Select your `kafka-logs-*` index pattern from the dropdown

## Verify Data Flow

### Check Logstash is consuming
```
docker logs logstash | grep -i kafka
docker logs logstash | grep -i elasticsearch
```

### Check Elasticsearch indices
```
curl "localhost:9200/_cat/indices?v"
curl "localhost:9200/kafka-logs-*/_mapping?pretty"
```

### Check if data exists
```
curl -X GET "localhost:9200/kafka-logs-*/_search?pretty" \
  -H 'Content-Type: application/json' \
  -d '{"size": 10}'
```

### Check Kafka topic has messages
```
docker exec -it kafka kafka-console-consumer \
  --bootstrap-server localhost:9092 \
  --topic logs \
  --from-beginning \
  --max-messages 10
```

### Check Logstash pipeline
```
curl "localhost:9600/_node/stats/pipelines?pretty"
```

### Send more test data
```
docker exec -it kafka kafka-console-producer \
  --bootstrap-server localhost:9092 \
  --topic logs
```
* Type JSON messages, one per line

## Quick Test Command

### Send test message
```
echo '{"message":"Test from Kafka","level":"INFO","service":"test"}' | \
  docker exec -i kafka kafka-console-producer \
  --bootstrap-server localhost:9092 \
  --topic logs
```

### Wait a few seconds, then check Elasticsearch
```
curl "localhost:9200/kafka-logs-*/_search?pretty&size=1"
```

### Consume from topic
```
docker exec -it kafka kafka-console-consumer --bootstrap-server kafka:9092 --topic logs
```

## References
* [Стриминг логов Kafka в Elasticsearch с визуализацией в Kibana](https://habr.com/ru/companies/simbirsoft/articles/972108/)
