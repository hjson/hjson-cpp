find_package(Threads REQUIRED)

add_executable(perfbin
  perf.cpp
  perf_multithread.cpp
)

target_compile_features(perfbin PUBLIC cxx_std_11)

target_link_libraries(perfbin hjson Threads::Threads)

add_custom_target(runperf
  COMMAND perfbin
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
