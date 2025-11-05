# Demo

## Create Redpanda topic
* `docker exec -it redpanda rpk topic create chat-room --brokers=localhost:9092`
* `docker exec -it redpanda rpk topic list`

## Redpanda node
```
#create and enter the project folder
mkdir redpanda-node
cd redpanda-node
#generate package.json
npm init
#install needed dependencies
npm i -g typescript
npm i -D @types/node
npm i kafkajs
npm i uuid
npm i -D @types/uuid
#generate tsconfig.json
tsc --init
```

### Docker
```
cd redpanda-node
docker build . -t demo_client
docker run -it --rm  --network demo_monitor-net demo_client
```

## Visualize metrics using Grafana

### Configure Prometheus Data Source in Grafana
Redpanda exposes Prometheus metrics to aid monitoring, but since we need to visualize the metrics in Grafana, we must first configure a Prometheus Data Source from which we would retrieve the metrics.
* Open your grafana UI at http://localhost:3000.
* Enter “admin” for username and password.
* Click on the "cogwheel" in the sidebar to open the Configuration menu. (Home > Connections)
* Click on "Data Sources."
* Click on "Add data source."
* Select "Prometheus" as the type.
* Name the datasource “prometheus.”
* Set the Prometheus server URL as http://prometheus:9090.
* Click "Save & Test" to save the new data source.

### Generate Grafana Dashboard
Generate a comprehensive Grafana dashboard to visualize monitoring metrics such as node health, errors, and latency. In the demo folder, run this command
```
docker exec -it redpanda rpk generate grafana-dashboard \
--datasource prometheus \
--metrics-endpoint otel-collector:8889/metrics > redpanda-dashboard.json
```

### Import Dashboard: 
Import the generated dashboard file into Grafana for visualization.

* Under the dashboard icon in your Grafana UI, click “Import.”
* Import the generated redpanda-dashboard.json file from your laptop.
* Click “Import”
* Send messages between the two terminals opened before and relevant metrics will reflect in the graphs.


