cmake_minimum_required(VERSION 3.1)

if (TARGET ocpn::cpl)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
endif ()

set(SRC
  src/cpl_vsi.h
  src/cpl_error.h
  src/cpl_config.h
  src/cpl_csv.h
  src/cpl_port.h
  src/cpl_string.h
  src/cpl_conv.h
  src/cpl_findfile.cpp
  src/cpl_vsisimple.cpp
  src/cpl_error.cpp
  src/cpl_path.cpp
  src/cpl_conv.cpp
  src/cpl_csv.cpp
  src/cpl_string.cpp
)   

# Library is used also in the plugins, so:
set(CMAKE_POSITION_INDEPENDENT_CODE "ON")

add_library(CPL STATIC ${SRC})

add_library(ocpn::cpl ALIAS CPL)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET CPL PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
target_include_directories(CPL PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
