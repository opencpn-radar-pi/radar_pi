# ~~~
# Summary:     Cmake toolchain file for 32-bit android armhf
# License:     GPLv3+
# Copyright (c) 2021 Alec Leamas
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 21)
set(CMAKE_ANDROID_ARCH_ABI armeabi-v7a)
if (DEFINED ENV{NDK_HOME})
  set(CMAKE_ANDROID_NDK $ENV{NDK_HOME})
else ()
  set(CMAKE_ANDROID_NDK /opt/android/ndk)
endif ()
set(ARM_ARCH armhf CACHE STRING "Selected arm architecture" FORCE)
