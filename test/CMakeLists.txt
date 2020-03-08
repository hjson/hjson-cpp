add_executable(testbin
  hjson_test.h
  test.cpp
  test_marshal.cpp
  test_value.cpp
)

target_compile_features(testbin PUBLIC cxx_std_11)

target_link_libraries(testbin hjson)

add_custom_target(runtest
  COMMAND testbin
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
