# ~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
# ~~~

set(PLUGIN_SOURCE_DIR .)

# This should be 2.8.0 to have FindGTK2 module

message(STATUS "*** Staging to build ${PACKAGE_NAME} ***")

set(PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# SET(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)

# SET(PROFILING 1)

# IF NOT DEBUGGING CFLAGS="-O2 -march=native"
if (NOT MSVC)
  if (PROFILING)
    add_definitions("-Wall -g -fprofile-arcs -ftest-coverage -fexceptions")
  else (PROFILING)
    # ADD_DEFINITIONS( "-Wall -g -fexceptions" )
    add_definitions("-Wall -Wno-unused-result -g -O2 -fexceptions -fPIC")
  endif (PROFILING)

  if (NOT APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
  else (NOT APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl -undefined dynamic_lookup")
  endif (NOT APPLE)

endif (NOT MSVC)

# Add some definitions to satisfy MS
if (MSVC)
  add_definitions(-D__MSVC__)
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE)
endif (MSVC)

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
set(BUILD_SHARED_LIBS "ON")

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

if (MSYS)
  # this is just a hack. I think the bug is in FindwxWidgets.cmake
  string(REGEX REPLACE "/usr/local" "\\\\;C:/MinGW/msys/1.0/usr/local"
                       wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS}
  )
endif (MSYS)

# QT_ANDROID is a cross-build, so the native FIND_PACKAGE(OpenGL) is not useful.
#
if (NOT QT_ANDROID)
  find_package(OpenGL)
  if (OPENGL_GLU_FOUND)

    set(wxWidgets_USE_LIBS ${wxWidgets_USE_LIBS} gl)
    include_directories(${OPENGL_INCLUDE_DIR})

    message(STATUS "Found OpenGL...")
    message(STATUS "    Lib: " ${OPENGL_LIBRARIES})
    message(STATUS "    Include: " ${OPENGL_INCLUDE_DIR})
    add_definitions(-DocpnUSE_GL)
  else (OPENGL_GLU_FOUND)
    message(STATUS "OpenGL not found...")
  endif (OPENGL_GLU_FOUND)
endif (NOT QT_ANDROID)

# On Android, PlugIns need a specific linkage set....
if (QT_ANDROID)
  # These libraries are needed to create PlugIns on Android.

  set(OCPN_Core_LIBRARIES
      # Presently, Android Plugins are built in the core tree, so the variables
      # {wxQT_BASE}, etc.
      # flow to this module from above.  If we want to build Android plugins
      # out-of-core, this will need improvement.
      # TODO This is pretty ugly, but there seems no way to avoid specifying a
      # full path in a cross build....
      /home/dsr/Projects/opencpn_sf/opencpn/build-opencpn-Android_for_armeabi_v7a_GCC_4_8_Qt_5_5_0-Debug/libopencpn.so
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_baseu-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_core-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_html-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_baseu_xml-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_qa-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_adv-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_aui-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_baseu_net-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_gl-3.1-arm-linux-androideabi.a
      ${Qt_Base}/android_armv7/lib/libQt5Core.so
      ${Qt_Base}/android_armv7/lib/libQt5OpenGL.so
      ${Qt_Base}/android_armv7/lib/libQt5Widgets.so
      ${Qt_Base}/android_armv7/lib/libQt5Gui.so
      ${Qt_Base}/android_armv7/lib/libQt5AndroidExtras.so
      # ${NDK_Base}/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi-v7a/libgnustl_shared.so
  )

endif (QT_ANDROID)

set(BUILD_SHARED_LIBS TRUE)

find_package(Gettext REQUIRED)
