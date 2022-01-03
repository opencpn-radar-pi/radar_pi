cmake_minimum_required(VERSION 3.1)

if (TARGET android::headers)
  return ()
endif ()

add_library(_android_headers INTERFACE)
add_library(android::headers ALIAS _android_headers)
target_include_directories(_android_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
