# ~~~
# Summary:      Set up the compilation environment
# Author:       Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright (c) 2014 Pavel Kallian
#               2021 Alec Leamas
#
# Set up the compilation environment, compiler options etc.
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS TRUE)

set(_ocpn_cflags " -Wall -Wno-unused-result -fexceptions")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(APPEND CMAKE_C_FLAGS " ${_ocpn_cflags}")
  string(APPEND CMAKE_CXX_FLAGS " ${_ocpn_cflags}")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-Bsymbolic")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")           # Apple is AppleClang
  string(APPEND CMAKE_C_FLAGS " ${_ocpn_cflags}")
  string(APPEND CMAKE_CXX_FLAGS " ${_ocpn_cflags}")
  string(APPEND CMAKE_CXX_FLAGS " -Wno-inconsistent-missing-override")
  string(APPEND CMAKE_CXX_FLAGS " -Wno-potentially-evaluated-expression")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl -undefined dynamic_lookup")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE)
endif ()

if (UNIX AND NOT APPLE)   # linux, see OpenCPN/OpenCPN#1977
  set_target_properties(${PACKAGE_NAME}
    PROPERTIES INSTALL_RPATH "$ORIGIN:$ORIGIN/.."
  )
endif ()

if (MINGW)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L../buildwin")
endif ()
