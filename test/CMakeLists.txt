add_executable(testbin
  hjson_test.h
  test.cpp
  test_marshal.cpp
  test_value.cpp
)

set_target_properties(testbin PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

target_link_libraries(testbin hjson)

add_custom_target(runtest
  COMMAND testbin
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
