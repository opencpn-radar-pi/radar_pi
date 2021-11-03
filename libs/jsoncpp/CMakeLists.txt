# ~~~
# Summary:      jsoncpp library cmake setup
# License:      Expat
# Copyright (c) 2021 Alec Leamas
# ~~~

if (TARGET ocpn::jsoncpp)
  return()
endif ()

cmake_minimum_required(VERSION 3.1.0)
project("jsoncpp")

option(USE_SYSTEM_JSONCPP "Use system copy of library if available" OFF)

add_library(_JSONCPP INTERFACE)
add_library(ocpn::jsoncpp ALIAS _JSONCPP)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)
find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h PATH_SUFFIXES jsoncpp)

if (JSONCPP_LIBRARY AND JSONCPP_INCLUDE_DIR AND USE_SYSTEM_JSONCPP)
  target_link_libraries(_JSONCPP INTERFACE ${JSONCPP_LIBRARY})
  target_include_directories(_JSONCPP INTERFACE ${JSONCPP_INCLUDE_DIR})
  message(STATUS "Using system jsoncpp")
  return ()
endif ()

if ((${CMAKE_VERSION} VERSION_GREATER "3.8.0" AND NOT WIN32)
  # FIXME: Always bundles large jsoncpp library, should use system when
  # available.
  AND NOT ${BUILD_TYPE} STREQUAL "tarball"
)
  include("CMakeLists-org.txt")
else ()
  set(SRC src/lib_json/json_reader.cpp
          src/lib_json/json_tool.h
          src/lib_json/json_value.cpp
          src/lib_json/json_writer.cpp
  )
  set(STATIC_LIB "JSONCPP_LIB")
  add_library(${STATIC_LIB} STATIC ${SRC})
  target_include_directories(
    ${STATIC_LIB} PRIVATE "${PROJECT_SOURCE_DIR}/include"
  )
endif ()

message(STATUS "Using bundled jsoncpp.")
target_link_libraries(_JSONCPP INTERFACE ${STATIC_LIB})
target_include_directories(_JSONCPP INTERFACE "${PROJECT_SOURCE_DIR}/include")
