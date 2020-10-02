# ~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
#
# Set up the compilation environment, compiler options etc.
# ~~~

message(STATUS "*** Staging to build ${PACKAGE_NAME} ***")

set(COMPILER ${CMAKE_CXX_COMPILER_ID})
if (COMPILER STREQUAL "Clang" OR COMPILER STREQUAL "GNU")
  # ADD_DEFINITIONS( "-Wall -g -fexceptions" )
  add_definitions("-Wall -Wno-unused-result -g -O2 -fexceptions")
elseif (COMPILER STREQUAL "GNU")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
elseif (COMPILER MATCHES   "Clang")              # Apple is AppleClang
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -Wl -undefined dynamic_lookup")
elseif (COMPILER STREQUAL "MSVC")
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE)
endif ()

# QT_ANDROID is a cross-build, so the native FIND_PACKAGE(wxWidgets...) and
# wxWidgets_USE_FILE is not useful.
if (NOT QT_ANDROID)
  if (NOT DEFINED wxWidgets_USE_FILE)
    set(wxWidgets_USE_LIBS base core net xml html adv)
    set(BUILD_SHARED_LIBS TRUE)
    find_package(wxWidgets REQUIRED)
  endif (NOT DEFINED wxWidgets_USE_FILE)

  include(${wxWidgets_USE_FILE})
endif (NOT QT_ANDROID)

if (UNIX AND NOT APPLE)   # linux, see OpenCPN/OpenCPN#1977
  set_target_properties(${PACKAGE_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN:$ORIGIN/..")
endif ()

if (NOT COMPILER STREQUAL "MSVC")
  set(OBJ_VISIBILITY "-fvisibility=hidden")
endif ()
