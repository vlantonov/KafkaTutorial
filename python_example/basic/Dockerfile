FROM python:3.10-slim-bullseye

ADD requirements.txt .
ADD consumer.py .
ADD producer.py .

RUN pip install -r requirements.txt

# Producer only (same for consumer)
# CMD ["python", "./producer.py"]
# Then run: docker run --network=host python-kafka

ENTRYPOINT ["python"]