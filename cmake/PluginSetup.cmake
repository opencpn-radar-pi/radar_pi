# ~~~
# Summary:     Set up and export plugin_target and plugin_target_version
# License:     GPLv3+
# Copyright (c) 2020-2021 Alec Leamas
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


if (DEFINED plugin_target)
  return ()
endif ()

if (NOT "${OCPN_TARGET_TUPLE}" STREQUAL "")
  list(GET OCPN_TARGET_TUPLE 0 plugin_target)
  list(GET OCPN_TARGET_TUPLE 1 plugin_target_version)
elseif ("${BUILD_TYPE}" STREQUAL "flatpak")
  set(plugin_target "flatpak")
  file(GLOB manifest_path "${PROJECT_SOURCE_DIR}/flatpak/org.opencpn.*.yaml")
  file(READ ${manifest_path} manifest)
  string(REPLACE "\n" ";" manifest_lines "${manifest}")
  foreach (_line ${manifest_lines})
    if (${_line} MATCHES  "org.freedesktop.Sdk")
      string(REGEX REPLACE ".*//" "" plugin_target_version "${_line}")
    endif ()
  endforeach ()
  message(STATUS  "Building for flatpak runtime ${plugin_target_version}")
elseif (MINGW)
  set(plugin_target "mingw")
  if (CMAKE_SYSTEM_VERSION)
    set(plugin_target_version ${CMAKE_SYSTEM_VERSION})
  else ()
    set(plugin_target_version 10)
  endif ()
elseif (MSVC)
  set(plugin_target "msvc-wx32")
  if (CMAKE_SYSTEM_VERSION)
    set(plugin_target_version ${CMAKE_SYSTEM_VERSION})
  elseif (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(plugin_target_version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
  else ()
    set(plugin_target_version 10)
  endif ()
elseif (APPLE)
  set(plugin_target "darwin-wx32")
  set(plugin_target_version "10.13.6")
elseif (UNIX)
  # Some linux dist:
  execute_process(
    COMMAND "lsb_release" "-is"
    OUTPUT_VARIABLE plugin_target
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND "lsb_release" "-rs"
    OUTPUT_VARIABLE plugin_target_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else ()
  set(plugin_target "unknown")
  set(plugin_target_version 1)
endif ()

string(STRIP "${plugin_target}" plugin_target)
string(TOLOWER "${plugin_target}" plugin_target)
string(STRIP "${plugin_target_version}" plugin_target_version)
string(TOLOWER "${plugin_target_version}" plugin_target_version)

if (plugin_target STREQUAL "ubuntu")
  if (DEFINED wxWidgets_CONFIG_EXECUTABLE)
    set(_WX_CONFIG_PROG ${wxWidgets_CONFIG_EXECUTABLE})
  else ()
    find_program(_WX_CONFIG_PROG NAMES $ENV{WX_CONFIG} wx-config )
  endif ()
  if (_WX_CONFIG_PROG)
    execute_process(
      COMMAND ${_WX_CONFIG_PROG} --selected-config
      OUTPUT_VARIABLE _WX_SELECTED_CONFIG
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (_WX_SELECTED_CONFIG MATCHES gtk3)
      if (${plugin_target_version} VERSION_LESS 22.04)
        set(plugin_target ubuntu-gtk3)
      endif ()
    endif ()
  else ()
    message(WARNING "Cannot locate wx-config utility")
  endif ()
endif ()

string(CONCAT msg "Building for target-release "
  "${plugin_target}-${plugin_target_version}"
)
message(STATUS "${msg}")
