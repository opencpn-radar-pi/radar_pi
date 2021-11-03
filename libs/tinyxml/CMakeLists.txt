# ~~~
# Summary:      tinyxml library cmake setup
# License:      GPLv3+
# Copyright (c) 2021 Alec Leamas
# ~~~

cmake_minimum_required(VERSION 3.1.0)

if (TARGET ocpn::tinyxml)
    return ()
endif ()

option(USE_SYSTEM_TINYXML "Use system copy of library if available" OFF)

set(SRC
  src/tinyxml.cpp
  src/tinyxmlerror.cpp
  src/tinyxmlparser.cpp
  include/tinyxml.h
)

find_library(TINYXML_LIBRARY NAMES tinyxml)
find_path(TINYXML_INCLUDE_DIR NAMES tinyxml.h PATH_SUFFIXES tinyxml)

add_library(TINYXML_IF INTERFACE)
add_library(ocpn::tinyxml ALIAS TINYXML_IF)

if (TINYXML_LIBRARY AND TINYXML_INCLUDE_DIR AND USE_SYSTEM_TINYXML)
  target_link_libraries(TINYXML_IF INTERFACE ${TINYXML_LIBRARY})
  target_include_directories(TINYXML_IF INTERFACE ${TINYXML_INCLUDE_DIR})
  message(STATUS "Using system tinyxml")
else ()
  add_library(TINYXML STATIC ${SRC})
  target_compile_definitions(TINYXML PUBLIC -DTIXML_USE_STL)
  target_include_directories(TINYXML
                             PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set_property(TARGET TINYXML PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
  endif ()
  target_link_libraries(TINYXML_IF INTERFACE TINYXML)
  message(STATUS "Using bundled tinyxml")
endif ()
