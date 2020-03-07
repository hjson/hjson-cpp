add_executable(perfbin
  perf.cpp
  perf_multithread.cpp
)

set_target_properties(perfbin PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

target_link_libraries(perfbin hjson)

add_custom_target(runperf
  COMMAND perfbin
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
