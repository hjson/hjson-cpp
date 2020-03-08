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

target_compile_features(hjson PUBLIC cxx_std_11)

set_target_properties(hjson PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  DEBUG_POSTFIX "d"
)

if(HJSON_ENABLE_INSTALL)
  install(TARGETS hjson EXPORT hjson DESTINATION ${lib_dest})
  install(FILES ${header} DESTINATION ${include_dest})
endif()
