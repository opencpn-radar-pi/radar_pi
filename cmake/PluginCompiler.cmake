# ~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
#
# Set up the compilation environment, compiler options etc.
# ~~~

message(STATUS "*** Staging to build ${PACKAGE_NAME} ***")

set(_ocpn_cflags " -Wall -Wno-unused-result -fexceptions")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(APPEND CMAKE_C_FLAGS " ${_ocpn_cflags}")
  string(APPEND CMAKE_CXX_FLAGS " ${_ocpn_cflags}")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-Bsymbolic")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")           # Apple is AppleClang
  string(APPEND CMAKE_C_FLAGS " ${_ocpn_cflags}")
  string(APPEND CMAKE_CXX_FLAGS " ${_ocpn_cflags}")
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



