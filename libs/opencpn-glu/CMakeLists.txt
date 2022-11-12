#
# Ensure that glu libraries and headers are present.
# - On Linux and Macos do nothing, the OS provides glu.
# - On Flatpak, bundle the header which corresponds the libglu
#   which comes with main OpenCPN
# - On Windows: Link to utility library and use WindowsHeaders.
# - On Android: Provide a header, qt libraries provide the functions.

cmake_minimum_required(VERSION 3.1)

if (TARGET opencpn::glu)
  return ()
endif ()

add_library(_glu_if INTERFACE)
add_library(opencpn::glu ALIAS _glu_if)

if (DEFINED ENV{FLATPAK_ID})
  target_include_directories(_glu_if INTERFACE include-flatpak)
elseif (WIN32)
  find_package(OpenGL)
  target_link_libraries(_glu_if INTERFACE OpenGL::GLU)
  target_link_libraries(_glu_if INTERFACE windows::headers)
elseif (QT_ANDROID)
  target_include_directories(_glu_if INTERFACE include-android)
else ()
  message(STATUS "Using system GL/glu.h and libraries")
endif ()
