add_executable(testbin
  hjson_test.h
  test.cpp
  test_marshal.cpp
  test_value.cpp
)

target_compile_features(testbin PUBLIC cxx_std_11)

if(${HJSON_NUMBER_PARSER} MATCHES "CharConv")
  target_compile_definitions(testbin PRIVATE HJSON_USE_CHARCONV=1)
elseif(${HJSON_NUMBER_PARSER} MATCHES "StrToD")
  target_compile_definitions(testbin PRIVATE HJSON_USE_STRTOD=1)
endif()

target_link_libraries(testbin hjson)

add_custom_target(runtest
  COMMAND testbin
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
