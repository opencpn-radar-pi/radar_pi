# ~~~
# Summary:      Windows headers library cmake setup
# License:      GPLv3+
# Copyright (c) 2021 Alec Leamas
# ~~~

cmake_minimum_required(VERSION 3.1)

if (TARGET windows::headers)
  return ()
endif ()

add_library(_windows_headers INTERFACE)
add_library(windows::headers ALIAS _windows_headers)
target_include_directories(
  _windows_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
