# ~~~
# Summary:      Find and link general plugin libraries
# License:      GPLv3+
# Copyright (c) 2021 Alec Leamas
#
# Find and link general libraries to use: gettext, wxWidgets and OpenGL
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

find_package(Gettext REQUIRED)

#
# Windows environment check
#
set(_bad_win_env_msg [=[
%WXWIN% is not present in environment, win_deps.bat has not been run.
Build might work, but most likely fail when not finding wxWidgets.
Run buildwin\win_deps.bat or set %WXWIN% to mute this message.
]=])

if (WIN32 AND NOT DEFINED ENV{WXWIN})
  message(WARNING ${_bad_win_env_msg})
endif ()

#
# OpenGL
#
# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")

find_package(OpenGL)
if (TARGET OpenGL::GL)
  target_link_libraries(${PACKAGE_NAME} OpenGL::GL)
else ()
  message(WARNING "Cannot locate usable OpenGL libs and headers.")
endif ()
if (NOT OPENGL_GLU_FOUND)
  message(WARNING "Cannot find OpenGL GLU extension.")
endif ()
if (APPLE)
  # As of 3.19.2, cmake's FindOpenGL does not link to the directory
  # containing gl.h. cmake bug? Intended due to missing subdir GL/gl.h?
  find_path(GL_H_DIR NAMES gl.h)
  if (GL_H_DIR)
    target_include_directories(${PACKAGE_NAME} PRIVATE "${GL_H_DIR}")
  else ()
    message(WARNING "Cannot locate OpenGL header file gl.h")
  endif ()
endif ()
if (WIN32)
  if (EXISTS "${PROJECT_SOURCE_DIR}/opencpn-libs/WindowsHeaders")
    add_subdirectory("${PROJECT_SOURCE_DIR}/opencpn-libs/WindowsHeaders")
    target_link_libraries(${PACKAGE_NAME} windows::headers)
  else ()
    message(STATUS
      "WARNING: WindowsHeaders library is missing, OpenGL unavailable"
    )
  endif ()
endif ()

#
# wxWidgets
#
set(wxWidgets_USE_DEBUG OFF)
set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_UNIVERSAL OFF)
set(wxWidgets_USE_STATIC OFF)

set(WX_COMPONENTS base core net xml html adv stc aui)
if (TARGET OpenGL::OpenGL OR TARGET OpenGL::GL)
  list(APPEND WX_COMPONENTS gl)
endif ()

find_package(wxWidgets REQUIRED ${WX_COMPONENTS})
include(${wxWidgets_USE_FILE})
target_link_libraries(${PACKAGE_NAME} ${wxWidgets_LIBRARIES})
