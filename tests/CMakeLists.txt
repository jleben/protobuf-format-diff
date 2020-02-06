
add_executable(run-tests test.cpp ../comparison.cpp)
target_link_libraries(run-tests protoc protobuf)

function(add_comparison_test dir_name)
  message(STATUS "Adding test ${dir_name}")
  add_test(NAME "${dir_name}" COMMAND
          run-tests "${dir_name}"
          WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

add_comparison_test(msg_added)
add_comparison_test(msg_removed)
add_comparison_test(enum_added)
add_comparison_test(enum_removed)
add_comparison_test(enum_value_added)
add_comparison_test(enum_value_removed)
add_comparison_test(enum_value_id_changed)
add_comparison_test(field_added)
add_comparison_test(field_removed)
add_comparison_test(field_id_changed)
add_comparison_test(field_label_changed)
add_comparison_test(field_type_changed)
add_comparison_test(field_default_value_changed)
add_comparison_test(field_message_type_name_changed)
add_comparison_test(field_message_type_changed)
add_comparison_test(field_enum_type_name_changed)
add_comparison_test(field_enum_type_changed)
add_comparison_test(msg_recursion)
