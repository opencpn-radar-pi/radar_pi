if (TARGET ocpn::jsoncpp)
  return()
endif ()

cmake_minimum_required(VERSION 3.1.0)
project("jsoncpp")
if (${CMAKE_VERSION} VERSION_GREATER "3.8.0" AND NOT WIN32)
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

add_library(_JSONCPP INTERFACE)
add_library(ocpn::jsoncpp ALIAS _JSONCPP)
target_link_libraries(_JSONCPP INTERFACE ${STATIC_LIB})
message(STATUS "JSONCPP_INCLUDE_DIR: ${JSONCPP_INCLUDE_DIR}")
target_include_directories(_JSONCPP INTERFACE "${PROJECT_SOURCE_DIR}/include")
