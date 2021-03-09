cmake_minimum_required(VERSION 3.5.0)

if (TARGET radar::garminhd)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
endif ()

SET(SRC
  include/garminhd/garminhdtype.h
  include/garminhd/GarminHDControlsDialog.h
  include/garminhd/GarminHDControlSet.h
  include/garminhd/GarminHDControl.h
  include/garminhd/GarminHDReceive.h

  src/GarminHDControl.cpp
  src/GarminHDControlsDialog.cpp
  src/GarminHDReceive.cpp
)

add_library(GARMINHD STATIC ${SRC})
add_library(radar::garminhd ALIAS GARMINHD)

target_link_libraries(GARMINHD PRIVATE
  radar::garminxhd radar::navico radar::raymarine ocpn::nmea0183 ocpn::wxjson ocpn::api
)

target_include_directories(GARMINHD PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(GARMINHD PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(GARMINHD PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/garminhd)

if (CMAKE_CX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET GARMINHD PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
