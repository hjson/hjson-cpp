set(header_path "${PROJECT_SOURCE_DIR}/include/hjson")
set(header ${header_path}/hjson.h)

set(src
  hjson_decode.cpp
  hjson_encode.cpp
  hjson_parsenumber.cpp
  hjson_value.cpp
)

add_library(hjson ${header} ${src})

target_include_directories(hjson PUBLIC
  $<BUILD_INTERFACE:${header_path}>
  $<INSTALL_INTERFACE:${include_dest}>
)

set_target_properties(hjson PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION 1
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
  DEBUG_POSTFIX "d"
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_INSTALLING)
  install(TARGETS hjson EXPORT hjson DESTINATION ${lib_dest})
  install(FILES ${header} DESTINATION ${include_dest})
endif()
