cmake_minimum_required(VERSION 3.1)

if (TARGET ocpn::iso8211)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
endif ()

set(SRC
  src/iso8211.h
  src/ddffield.cpp
  src/ddfsubfielddefn.cpp
  src/ddffielddefn.cpp
  src/ddfmodule.cpp
  src/ddfrecord.cpp
  src/ddfutils.cpp
)   

# Library is used also in the plugins, so:
set(CMAKE_POSITION_INDEPENDENT_CODE "ON")

add_library(ISO8211 STATIC ${SRC})
add_library(ocpn::iso8211 ALIAS ISO8211)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET ISO8211 PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
target_include_directories(ISO8211 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(ISO8211 PRIVATE ocpn::cpl)
