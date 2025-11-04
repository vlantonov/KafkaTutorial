# E-commerce example

## Architecture
                  +-------------------+
                  |    Client Apps    |
                  | (Web, Mobile, etc)|
                  +---------+---------+
                            |
                            | HTTP/HTTPS
                            |
                  +---------v---------+
                  |                   |
                  |    API Gateway    |
                  |     (Nginx)       |
                  |                   |
                  +----+------+-------+
                       |      |
           +-----------+      +-----------+
           |                              |
     +-----v------+  +-----------+  +-----v------+
     |            |  |           |  |            |
     |  Product   |  |   User    |  |   Order    |
     |  Service   |  |  Service  |  |  Service   |
     |   (Go)     |  | (Python)  |  | (Node.js)  |
     |            |  |           |  |            |
     +-----+------+  +-----+-----+  +-----+------+
           |              |              |
           |              |              |
    +------v--------------v--------------v------+
    |                                           |
    |              PostgreSQL                   |
    |              Database                     |
    |                                           |
    +-------------------------------------------+
                       |
                       | (Event Streaming)
                       |
               +-------v-------+
               |               |
               |   Redpanda    |
               |               |
               +-------+-------+
                       |
                       | (External Systems)
                       |
               +-------v-------+
               | Notifications |
               |   Analytics   |
               | Materialize   |
               | Other Services|
               +---------------+

## Running the Microservices
* `docker compose up -d`
* `docker compose logs -f`
* `docker compose down`

## Testing the Services
* Get products: `curl http://localhost/products`
* Get users: `curl http://localhost/users`
* Create an order: `curl -X POST http://localhost/orders -H "Content-Type: application/json" -d '{"productId": "1", "userId": "1", "quantity": 2}'`

## Scaling Services
* `docker compose up -d --scale product-service=3`
* `docker compose ps`
