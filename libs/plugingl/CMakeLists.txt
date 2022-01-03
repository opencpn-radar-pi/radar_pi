# ~~~
# Summary:      Plugin OpenGL support library cmake setup
# License:      GPLv3+
# Copyright (c) 2021 Alec Leamas
# ~~~

cmake_minimum_required(VERSION 3.1)

if (TARGET ocpn::plugingl)
    return ()
endif ()

set(SRC
  src/pi_shaders.cpp
  src/qtstylesheet.cpp
  src/TexFont.cpp
  include/linmath.h
  include/pi_shaders.h
  include/qtstylesheet.h
  include/TexFont.h
)   

# Library is used also in the plugins, so:
set(CMAKE_POSITION_INDEPENDENT_CODE "ON")

add_library(PLUGINGL STATIC ${SRC})
add_library(ocpn::plugingl ALIAS PLUGINGL)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET PLUGINGL PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
target_link_libraries(PLUGINGL PRIVATE ocpn::api)

if (WIN32)
target_link_libraries(PLUGINGL PRIVATE windows::headers)
endif ()

target_include_directories(PLUGINGL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
