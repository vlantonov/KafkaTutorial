cmake_minimum_required(VERSION 3.10)

project(KafkaSample LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 rdkafka)

# Copy configuration file
configure_file(getting-started.ini . COPYONLY)

# Producer
add_executable(
    KafkaProducer
    common.h
    common.c
    producer.c
)

set_target_properties(
    KafkaProducer
    PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        LINKER_LANGUAGE C
)

target_compile_options(
    KafkaProducer
    PUBLIC
         -Wall -Wextra -Wpedantic
)

target_link_libraries(
    KafkaProducer
    PUBLIC
        PkgConfig::deps
)

# Consumer
add_executable(
    KafkaConsumer
    common.h
    common.c
    consumer.c
)

set_target_properties(
    KafkaConsumer
    PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        LINKER_LANGUAGE C
)

target_compile_options(
    KafkaConsumer
    PUBLIC
         -Wall -Wextra -Wpedantic
)

target_link_libraries(
    KafkaConsumer
    PUBLIC
        PkgConfig::deps
)

