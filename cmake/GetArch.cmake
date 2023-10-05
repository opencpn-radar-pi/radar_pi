# ~~~
# Summary:      Set ARCH using cmake probes and various heuristics.
# License:      GPLv3+
# Copyright (c) 2021 Alec Leamas
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


if (COMMAND GetArch)
  return()
endif ()

# Based on code from nohal
function (GetArch)
  if (NOT "${OCPN_TARGET_TUPLE}" STREQUAL "")
    # Return last element from tuple like "Android-armhf;16;armhf"
    list(GET OCPN_TARGET_TUPLE 2 ARCH)
    if(ARCH STREQUAL "universal")
      set(ARCH "x86_64;arm64")
    endif()
  elseif (NOT WIN32)
    # default
    set(ARCH "x86_64")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
      if (CMAKE_SIZEOF_VOID_P MATCHES "8")
        set(ARCH "arm64")
      else ()
        set(ARCH "armhf")
      endif ()
    else (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
      set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif ()
    if ("${BUILD_TYPE}" STREQUAL "flatpak")
      if (ARCH STREQUAL "arm64")
        set(ARCH "aarch64")
      endif ()
    elseif (EXISTS /etc/redhat-release)
      if (ARCH STREQUAL "arm64")
        set(ARCH "aarch64")
      endif ()
    elseif (EXISTS /etc/suse-release OR EXISTS /etc/SuSE-release)
      if (ARCH STREQUAL "arm64")
        set(ARCH "aarch64")
      endif ()
    endif ()
  else (NOT WIN32)
    # Should really be i386 since we are on win32. However, it's x86_64 for now,
    # see #2027
    set(ARCH "x86_64")
  endif ()
  set(ARCH ${ARCH} PARENT_SCOPE)
endfunction (GetArch)

getarch()
