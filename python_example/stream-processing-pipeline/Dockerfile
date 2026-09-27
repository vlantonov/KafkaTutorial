FROM python:3.11-slim

WORKDIR /srv

# Install dependencies first for better layer caching.
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY app/ ./app/

# Faust needs unbuffered output so logs stream to `docker compose logs`.
ENV PYTHONUNBUFFERED=1
