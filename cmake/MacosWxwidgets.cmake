# ~~~
# Summary:     If required, rebuild wxwidgets for macos from source.
# License:     GPLv3+
# Copyright (c) 2022 Alec Leamas
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.20.0)

execute_process(
  COMMAND wx-config --version
  RESULT_VARIABLE wx_status
  ERROR_FILE /dev/null
  COMMAND_ECHO STDOUT
)

if (${wx_status} EQUAL 0)
  return ()
endif ()

include(FetchContent)

FetchContent_Declare(
  wxwidgets
  GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
  GIT_TAG v3.2.1
)
FetchContent_Populate(wxwidgets)
FetchContent_GetProperties(wxwidgets SOURCE_DIR wxwidgets_src_dir)

execute_process(
  COMMAND git submodule update --init 3rdparty/pcre
  WORKING_DIRECTORY ${wxwidgets_src_dir}
)
execute_process(
 COMMAND ./configure
      --with-cxx=11
      --with-macosx-version-min=10.10
      --enable-unicode
      --with-osx-cocoa
      --enable-aui
      --disable-debug
      --with-opengl
      --without-subdirs
      --prefix=/usr/local
 WORKING_DIRECTORY ${wxwidgets_src_dir}
)
math(_nproc ${OPCN_NPROC} * 2)    # Assuming two threads/cpu
execute_process(
  COMMAND make -j${_nproc}
  WORKING_DIRECTORY ${wxwidgets_src_dir}
)
execute_process(
  COMMAND sudo make install
  WORKING_DIRECTORY ${wxwidgets_src_dir}
)
