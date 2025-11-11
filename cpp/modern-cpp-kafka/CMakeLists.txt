cmake_minimum_required(VERSION 3.28)
project(modern_cpp_kafka_example LANGUAGES CXX)

find_package(modern-cpp-kafka REQUIRED)

add_executable(
    ${PROJECT_NAME}_producer 
        producer.cpp
)

set_target_properties(
    ${PROJECT_NAME}_producer 
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(
    ${PROJECT_NAME}_producer 
    PRIVATE 
        modern-cpp-kafka::modern-cpp-kafka
)

add_executable(
    ${PROJECT_NAME}_consumer 
        consumer.cpp
)

set_target_properties(
    ${PROJECT_NAME}_consumer 
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(
    ${PROJECT_NAME}_consumer 
    PRIVATE 
        modern-cpp-kafka::modern-cpp-kafka
)
