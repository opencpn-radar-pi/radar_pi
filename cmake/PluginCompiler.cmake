# ~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
#
# Set up the compilation environment, compiler options etc.
# ~~~

message(STATUS "*** Staging to build ${PACKAGE_NAME} ***")

set(COMPILER ${CMAKE_CXX_COMPILER_ID})
if (COMPILER STREQUAL "GNU")
  add_definitions("-Wall -Wno-unused-result -fexceptions")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
elseif (COMPILER MATCHES   "Clang")              # Apple is AppleClang
  add_definitions("-Wall -Wno-unused-result -fexceptions")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -Wl -undefined dynamic_lookup")
elseif (COMPILER STREQUAL "MSVC")
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE)
endif ()

if (UNIX AND NOT APPLE)   # linux, see OpenCPN/OpenCPN#1977
  set_target_properties(${PACKAGE_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN:$ORIGIN/..")
endif ()

if (NOT COMPILER STREQUAL "MSVC")
  set(OBJ_VISIBILITY "-fvisibility=hidden")
endif ()