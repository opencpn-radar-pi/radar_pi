# ~~~
# Summary:     Set up default plugin build options 
# License:     GPLv3+
# Copyright (c) 2020-2021 Alec Leamas
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

if (DEFINED _default_build_type)
  return ()
endif ()


# Set a default build type if none was specified
# https://blog.kitware.com/cmake-and-the-default-build-type/
set(_default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(_default_build_type "Debug")
endif()
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS
    "Setting build type to '${_default_build_type}' as none was specified."
  )
  set(CMAKE_BUILD_TYPE "${_default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


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


# Set up OCPN_TARGET_TUPLE and update QT_ANDROID accordingly
set(OCPN_TARGET_TUPLE "" CACHE STRING
  "Target spec: \"platform;version;arch\""
)

string(TOLOWER "${OCPN_TARGET_TUPLE}" _lc_target)
if ("${_lc_target}" MATCHES "android*")
  set(QT_ANDROID ON)
  if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR
      "Android targets requires using a toolchain file. See INSTALL.md"
    )
  endif ()
else ()
  set(QT_ANDROID OFF)
endif ()
