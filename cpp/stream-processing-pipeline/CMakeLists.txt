cmake_minimum_required(VERSION 3.16)
project(stream_processing_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKACPP REQUIRED rdkafka++)

find_package(Threads REQUIRED)

foreach(app producer processor verifier)
  add_executable(${app} src/${app}.cpp)
  target_include_directories(${app} PRIVATE ${RDKAFKACPP_INCLUDE_DIRS})
  target_link_directories(${app} PRIVATE ${RDKAFKACPP_LIBRARY_DIRS})
  target_link_libraries(${app} PRIVATE ${RDKAFKACPP_LIBRARIES} Threads::Threads)
endforeach()
