cmake_minimum_required(VERSION 3.1)

project(hjson)

if(MSVC)
  set(BUILD_WITH_STATIC_CRT "Manual flags" CACHE STRING "Use staticaly linked CRT")
  set_property(CACHE BUILD_WITH_STATIC_CRT PROPERTY STRINGS "Manual flags" "Yes" "No")
  include(cmake/msvc_crt.cmake)
endif(MSVC)

set(HJSON_VERSION_MAJOR 1 CACHE STRING "major version" FORCE)
set(HJSON_VERSION_MINOR 2 CACHE STRING "minor version" FORCE)
set(HJSON_VERSION ${HJSON_VERSION_MAJOR}.${HJSON_VERSION_MINOR} CACHE STRING "version" FORCE)

option(HJSON_ENABLE_TEST "Enable testing" OFF)
option(HJSON_ENABLE_INSTALL "Enable installation" OFF)
option(HJSON_VERSIONED_INSTALL "Include version in installation path" OFF)

if(HJSON_VERSIONED_INSTALL)
  set(include_dest "include/hjson-${HJSON_VERSION}")
  set(lib_dest "lib/hjson-${HJSON_VERSION}")
else()
  set(include_dest "include/hjson")
  set(lib_dest "lib/hjson")
endif()

add_subdirectory(src)
if(HJSON_ENABLE_TEST)
  add_subdirectory(test)
endif()

configure_file(cmake/hjson-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/hjson-config.cmake @ONLY)
configure_file(cmake/hjson-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/hjson-config-version.cmake @ONLY)

export(TARGETS hjson FILE "${CMAKE_BINARY_DIR}/hjson.cmake")

if(HJSON_ENABLE_INSTALL)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hjson-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hjson-config-version.cmake
    DESTINATION ${lib_dest}
  )

  install(EXPORT hjson DESTINATION ${lib_dest})
endif()
