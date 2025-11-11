cmake_minimum_required(VERSION 3.28)
project(cppkafka_example LANGUAGES CXX)

find_package(CppKafka REQUIRED)

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
        CppKafka::cppkafka
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
        CppKafka::cppkafka
)
