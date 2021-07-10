# ~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
#
# Set up the compilation environment, compiler options etc.
# ~~~


# Set up option NPROC: Number of processors used when compiling.
if (DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL})
  set(_nproc $ENV{CMAKE_BUILD_PARALLEL_LEVEL})
else ()
  if (WIN32)
    set(_nproc_cmd
      cmd /C if defined NUMBER_OF_PROCESSORS (echo %NUMBER_OF_PROCESSORS%)
      else (echo 1)
    )

  elseif (APPLE)
    set(_nproc_cmd sysctl -n hw.physicalcpu)
  else ()
    set(_nproc_cmd nproc)
  endif ()
  execute_process(
    COMMAND ${_nproc_cmd}
    OUTPUT_VARIABLE _nproc
    RESULT_VARIABLE _status
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (NOT "${_status}" STREQUAL "0")
    set(_nproc 1)
    message(
      STATUS "Cannot probe for processor count using \"${_nproc_cmd}\""
    )
  endif ()
endif ()
set(OCPN_NPROC ${_nproc}
  CACHE STRING "Number of processors used to compile [${_nproc}]"
)
message(STATUS "Build uses ${OCPN_NPROC} processors")

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
