# Build the three C++ stream-processing binaries against librdkafka, then ship
# them on a slim runtime image. The same image is reused for producer,
# processor, and verifier services (differentiated by the compose command).
FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        pkg-config \
        librdkafka-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt ./
COPY src ./src
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        librdkafka++1 \
        librdkafka1 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /app/build/producer /usr/local/bin/producer
COPY --from=build /app/build/processor /usr/local/bin/processor
COPY --from=build /app/build/verifier /usr/local/bin/verifier

CMD ["processor"]
