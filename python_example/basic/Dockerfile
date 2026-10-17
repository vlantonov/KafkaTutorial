FROM python:3.10-slim-bullseye

ADD requirements.txt .
ADD consumer.py .
ADD producer.py .
ADD transactional_producer.py .
ADD rebalance_consumer.py .
ADD schema_registry_avro_demo.py .
ADD schema_registry_protobuf_demo.py .

RUN pip install -r requirements.txt

# Producer only (same for consumer)
# CMD ["python", "./producer.py"]
# Then run: docker run --network=host python-kafka

ENTRYPOINT ["python"]