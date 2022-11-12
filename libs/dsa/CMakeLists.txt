cmake_minimum_required(VERSION 3.1)

if (TARGET ocpn::dsa)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
endif ()

set(SRC
  src/mp_math.c
  src/sha1.c
  src/mp_math.h
  src/dsa_utils.h
  src/sha1.h
  src/dsa_utils.cpp
)   

# Library is used also in the plugins, so:
set(CMAKE_POSITION_INDEPENDENT_CODE "ON")

add_library(DSA STATIC ${SRC})
add_library(ocpn::dsa ALIAS DSA)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET DSA PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
target_include_directories(DSA PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
