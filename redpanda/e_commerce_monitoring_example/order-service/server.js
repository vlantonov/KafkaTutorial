const express = require('express');
const { Kafka } = require('kafkajs');

const app = express();
const kafka = new Kafka({
  clientId: 'order-service',
  brokers: ['redpanda:9092']
});

const producer = kafka.producer();

app.use(express.json());

app.post('/orders', async (req, res) => {
  const order = req.body;
  await producer.connect();
  await producer.send({
    topic: 'orders',
    messages: [{ value: JSON.stringify(order) }],
  });
  await producer.disconnect();
  res.status(201).json({ message: 'Order created' });
});

app.listen(8082, () => console.log('Order service listening on port 8082'));
