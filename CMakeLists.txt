cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.15)
endif()

include_guard(GLOBAL)

project(hjson
  VERSION 2.0
  DESCRIPTION "Human readable JSON"
  LANGUAGES CXX)

if(MSVC)
  set(BUILD_WITH_STATIC_CRT "Manual flags" CACHE STRING "Use statically linked CRT")
  set_property(CACHE BUILD_WITH_STATIC_CRT PROPERTY STRINGS "Manual flags" "Yes" "No")
  include(cmake/msvc_crt.cmake)
endif(MSVC)

option(HJSON_ENABLE_TEST "Enable testing" OFF)
option(HJSON_ENABLE_PERFTEST "Enable performance testing" OFF)
option(HJSON_ENABLE_INSTALL "Enable installation" OFF)
option(HJSON_VERSIONED_INSTALL "Include version in installation path" OFF)
set(HJSON_NUMBER_PARSER "StringStream" CACHE STRING "Which number parsing tool to use")
set_property(CACHE HJSON_NUMBER_PARSER PROPERTY STRINGS "StringStream" "StrToD" "CharConv")
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS "Needed for shared libs on Windows" ON)

if(HJSON_VERSIONED_INSTALL)
  set(include_dest "include/hjson-${PROJECT_VERSION}")
  set(cmake_dest "lib/hjson-${PROJECT_VERSION}")
  set(lib_dest "lib/hjson-${PROJECT_VERSION}")
else()
  set(include_dest "include/hjson")
  set(cmake_dest "lib/hjson")
  set(lib_dest "lib")
endif()

add_subdirectory(src)
if(HJSON_ENABLE_TEST)
  add_subdirectory(test)
endif()
if(HJSON_ENABLE_PERFTEST)
  add_subdirectory(performance)
endif()

configure_file(cmake/hjson-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/hjson-config.cmake @ONLY)
configure_file(cmake/hjson-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/hjson-config-version.cmake @ONLY)

export(TARGETS hjson FILE "${CMAKE_BINARY_DIR}/hjson.cmake")

if(HJSON_ENABLE_INSTALL)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hjson-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hjson-config-version.cmake
    DESTINATION ${cmake_dest}
  )

  install(EXPORT hjson DESTINATION ${cmake_dest})
endif()
