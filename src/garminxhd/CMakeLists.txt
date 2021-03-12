cmake_minimum_required(VERSION 3.5.0)

if (TARGET radar::garminxhd)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
endif ()

SET(SRC
  include/garminxhd/garminxhdtype.h
  include/garminxhd/GarminxHDControl.h
  include/garminxhd/GarminxHDControlsDialog.h
  include/garminxhd/GarminxHDControlSet.h
  include/garminxhd/GarminxHDReceive.h

  src/GarminxHDControl.cpp
  src/GarminxHDControlsDialog.cpp
  src/GarminxHDReceive.cpp
)

add_library(GARMINXHD STATIC ${SRC})
add_library(radar::garminxhd ALIAS GARMINXHD)

target_link_libraries(GARMINXHD PRIVATE
  radar::garminhd radar::navico radar::raymarine ocpn::wxjson ocpn::nmea0183 ocpn::api
)

if (CMAKE_CX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET GARMINXHD PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()

target_include_directories(GARMINXHD PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(GARMINXHD PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/garminxhd)
