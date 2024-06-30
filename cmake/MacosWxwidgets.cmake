# ~~~
# Summary:     If required, rebuild wxwidgets for macos from source.
# License:     GPLv3+
# Copyright (c) 2022 Alec Leamas
# ~~~

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.

cmake_minimum_required(VERSION 3.20.0)

set(wx_repo https://github.com/wxWidgets/wxWidgets.git)
set(wx_tag v3.2.2.1)

option(IGNORE_SYSTEM_WX "Never use system wxWidgets installation" FALSE)

# Check if we have done the wxWidgets build already
#
if(DEFINED wx_config)
  return()
endif()

# Check if there is a usable wxwidgets anyway
#
set(cache_dir ${PROJECT_SOURCE_DIR}/cache)

if(IGNORE_SYSTEM_WX)
  set(WX_CONFIG_PROG ${cache_dir}/lib/wx/config/osx_cocoa-unicode-3.2)
else()
  find_program(
    WX_CONFIG_PROG
    NAMES wx-config osx_cocoa-unicode-3.2
    HINTS ${PROJECT_SOURCE_DIR}/cache/lib/wx/config /usr/local/lib/wx/config)
endif()
if(WX_CONFIG_PROG)
  execute_process(
    COMMAND ${WX_CONFIG_PROG} --version
    RESULT_VARIABLE wx_status
    OUTPUT_VARIABLE wx_version
    ERROR_FILE /dev/null COMMAND_ECHO STDOUT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  set(wx_status 1)
endif()

if(${wx_status} EQUAL 0)
  set(wx_config
      ${WX_CONFIG_PROG}
      CACHE FILEPATH "")
  set(ENV{WX_CONFIG} ${WX_CONFIG_PROG})
  if(${wx_version} VERSION_GREATER_EQUAL 3.2)
    return()
  endif()
endif()

if(NOT EXISTS ${cache_dir})
  file(MAKE_DIRECTORY ${cache_dir})
endif()

# Download sources and get the source directory
#
include(FetchContent)
FetchContent_Declare(
  wxwidgets
  GIT_REPOSITORY ${wx_repo}
  GIT_TAG ${wx_tag})
FetchContent_Populate(wxwidgets)
FetchContent_GetProperties(wxwidgets SOURCE_DIR wxwidgets_src_dir)

execute_process(COMMAND git submodule update --init 3rdparty/pcre
                WORKING_DIRECTORY ${wxwidgets_src_dir})
execute_process(
  COMMAND
    ./configure --with-cxx=11 --with-macosx-version-min=10.10 --enable-unicode
    --with-osx-cocoa --enable-aui --disable-debug --with-opengl
    --enable-macosx_arch=arm64,x86_64 --enable-universal_binary=arm64,x86_64
    --without-subdirs --prefix=${cache_dir}
  WORKING_DIRECTORY ${wxwidgets_src_dir})
math(_nproc ${OCPN_NPROC} * 2) # Assuming two threads/cpu
execute_process(COMMAND make -j${_nproc} WORKING_DIRECTORY ${wxwidgets_src_dir})
execute_process(COMMAND sudo make install
                WORKING_DIRECTORY ${wxwidgets_src_dir})

set(wx_config ${cache_dir}/lib/wx/config/osx_cocoa-unicode-3.2)
if(NOT EXISTS ${wx_config})
  message(FATAL_ERROR "Cannot locate wx-config tool at ${wx_config}")
endif()
set(ENV{WX_CONFIG} ${wx_config})
