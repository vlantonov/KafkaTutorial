import json
from datetime import datetime

from confluent_kafka import KafkaException, Producer


def delivery_report(err, msg):
    if err is not None:
        print(f"Delivery failed for key={msg.key()}: {err}")
        return

    print(
        "Delivered key="
        f"{msg.key().decode('utf-8')} to {msg.topic()}[{msg.partition()}] @ {msg.offset()}"
    )


def main():
    producer = Producer(
        {
            "bootstrap.servers": "localhost:9093",
            "transactional.id": "python-eos-producer-1",
            "enable.idempotence": True,
            "acks": "all",
        }
    )

    topic = "my_test_topic"

    print("Initializing transactions...")
    producer.init_transactions(10.0)

    print("Starting transaction...")
    producer.begin_transaction()
    try:
        for i in range(5):
            payload = {
                "author": "author_name",
                "content": f"Exactly-once message #{i}",
                "created_at": datetime.now().strftime("%Y-%m-%dT%H:%M:%S"),
            }
            producer.produce(
                topic=topic,
                key=str(i),
                value=json.dumps(payload),
                callback=delivery_report,
            )
            producer.poll(0)

        print("Committing transaction...")
        producer.commit_transaction(10.0)
        print("Transaction committed successfully.")
    except KafkaException as exc:
        error = exc.args[0]
        print(f"Transaction failed: {error}")

        if error.txn_requires_abort():
            print("Aborting transaction...")
            producer.abort_transaction(10.0)
            print("Transaction aborted.")
        raise


if __name__ == "__main__":
    main()
