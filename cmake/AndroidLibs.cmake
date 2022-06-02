# ~~~
# Summary:      Wraps the master.zip binary Android libs blob
# License:      GPLv3+
# Copyright (c) 2021 Alec Leamas
#
# For android armhf and arm64: Download precompiled libraries and set up
# linkage
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


cmake_minimum_required(VERSION 3.1)

find_package(Gettext REQUIRED)

# install() needs to find the cross-compiled library:
set_property(
  TARGET ${PACKAGE_NAME}
  PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib${PACKAGE_NAME}.so
)

# Make sure we have downloaded and unpacked master.zip
set(
  OCPN_ANDROID_CACHEDIR "${CMAKE_SOURCE_DIR}/cache"
  CACHE STRING "Build download area"
)
set(_master_base ${OCPN_ANDROID_CACHEDIR}/OCPNAndroidCommon-master)

if (NOT EXISTS ${OCPN_ANDROID_CACHEDIR}/master.zip)
  file(
    DOWNLOAD
      https://github.com/bdbcat/OCPNAndroidCommon/archive/master.zip
      ${OCPN_ANDROID_CACHEDIR}/master.zip
    EXPECTED_HASH
      SHA256=a15ebd49fd4e7c0f2d6e99328ed6a9fdb8ed7c8fcaa993cab585fc9d8aab4f56
    SHOW_PROGRESS
  )
endif ()
if (NOT EXISTS ${_master_base})
  message(STATUS "Extracting image (patience, please...)")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar -xzf ${OCPN_ANDROID_CACHEDIR}/master.zip
    WORKING_DIRECTORY "${OCPN_ANDROID_CACHEDIR}"
  )
endif ()

# Set up Qt_Build and wxQt_Build
if ("${ARM_ARCH}" STREQUAL "aarch64")
  set(_Qt_Build  "build_arm64/qtbase")
  set(_wxQt_Build "build_android_release_64_static_O3")
elseif ("${ARM_ARCH}" STREQUAL "armhf")
  set(_Qt_Build  "build_arm32_19_O3/qtbase")
  set(_wxQt_Build "build_android_release_19_static_O3")
else ()
  message(FATAL_ERROR "No valid arm configuration detected.")
endif ()
set(Qt_Build  ${_Qt_Build} CACHE STRING "Base directory for QT build")
set(wxQt_Build ${_wxQt_Build} CACHE STRING "wxWidgets QT build base directory")

# Setup directories and libraries
if ("${Qt_Build}" MATCHES "arm64")
  file(GLOB _wx_setup
    ${_master_base}/wxWidgets/libarm64/wx/include/arm-linux-*-static-*
  )
  set(_qt_include  ${_master_base}/qt5/build_arm64_O3/qtbase/include)
  set(_libgorp ${_master_base}/opencpn/API-117/libarm64/libgorp.so)
  set(_qtlibs  ${_master_base}/qt5/build_arm64_O3/qtbase/lib)
else ()
  file(GLOB _wx_setup
    ${_master_base}/wxWidgets/libarmhf/wx/include/arm-linux-*-static-*
  )
  set(_qt_include ${_master_base}/qt5/build_arm32_19_O3/qtbase/include)
  set(_libgorp ${_master_base}/opencpn/API-117/libarmhf/libgorp.so)
  set(_qtlibs  ${_master_base}/qt5/build_arm32_19_O3/qtbase/lib)
endif ()

include_directories(
  ${_qt_include}
  ${_qt_include}/QtWidgets
  ${_qt_include}/QtCore
  ${_qt_include}/QtGui
  ${_qt_include}/QtOpenGL
  ${_qt_include}/QtTest
  ${_master_base}/wxWidgets/include/
  ${_wx_setup}
)
target_link_libraries(${PACKAGE_NAME}
  ${_libgorp}
  ${_qtlibs}/libQt5Core.so
  ${_qtlibs}/libQt5OpenGL.so
  ${_qtlibs}/libQt5Widgets.so
  ${_qtlibs}/libQt5Gui.so
  ${_qtlibs}/libQt5AndroidExtras.so
  libGLESv2.so
  libEGL.so
)
add_compile_definitions(
  __WXQT__
  BUILDING_PLUGIN
  OCPN_USE_WRAPPER
  ocpnUSE_GLES
  ocpnUSE_GL
  USE_ANDROID_GLES2
  USE_GLSL
  QT_WIDGETS_LINUX
)
if (NOT CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -s")
endif ()
