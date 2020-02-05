cmake_minimum_required(VERSION 3.10)

project(protobuf-spec-comparator)

add_executable(protobuf-spec-compare comparison.cpp main.cpp)
target_link_libraries(protobuf-spec-compare protoc protobuf)

add_subdirectory(tests)
