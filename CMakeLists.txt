cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.15)
endif()

project(hjson
  VERSION 1.2
  DESCRIPTION "Human readable JSON"
  LANGUAGES CXX)

if(MSVC)
  set(BUILD_WITH_STATIC_CRT "Manual flags" CACHE STRING "Use statically linked CRT")
  set_property(CACHE BUILD_WITH_STATIC_CRT PROPERTY STRINGS "Manual flags" "Yes" "No")
  include(cmake/msvc_crt.cmake)
endif(MSVC)

set(HJSON_VERSION_MAJOR 1 CACHE STRING "major version" FORCE)
set(HJSON_VERSION_MINOR 2 CACHE STRING "minor version" FORCE)
set(HJSON_VERSION ${HJSON_VERSION_MAJOR}.${HJSON_VERSION_MINOR} CACHE STRING "version" FORCE)

option(BUILD_TESTING "Enable testing" OFF)
option(BUILD_INSTALLING "Enable installation" OFF)
option(HJSON_VERSIONED_INSTALL "Include version in installation path" OFF)

if(HJSON_VERSIONED_INSTALL)
  set(include_dest "include/hjson-${HJSON_VERSION}")
  set(lib_dest "lib/hjson-${HJSON_VERSION}")
else()
  set(include_dest "include/hjson")
  set(lib_dest "lib")
endif()

add_subdirectory(src)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_subdirectory(test)
endif()

configure_file(cmake/hjson-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/hjson-config.cmake @ONLY)
configure_file(cmake/hjson-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/hjson-config-version.cmake @ONLY)

export(TARGETS hjson FILE "${CMAKE_BINARY_DIR}/hjson.cmake")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_INSTALLING)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hjson-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hjson-config-version.cmake
    DESTINATION ${lib_dest}
  )

  install(EXPORT hjson DESTINATION ${lib_dest})
endif()
