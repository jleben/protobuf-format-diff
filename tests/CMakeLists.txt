
add_executable(run-tests test.cpp ../comparison.cpp)
target_link_libraries(run-tests protoc protobuf)
