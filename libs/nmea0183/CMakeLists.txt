cmake_minimum_required(VERSION 3.1.0)

if (TARGET ocpn::nmea0183)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
endif ()

SET(SRC
  src/expid.cpp
  src/hdg.cpp
  src/hdg.hpp
  src/hdm.cpp
  src/hdm.hpp
  src/hdt.cpp
  src/hdt.hpp
  src/hexvalue.cpp
  src/LatLong.hpp
  src/latlong.cpp
  src/long.cpp
  src/nmea0183.cpp
  src/nmea0183.h
  src/nmea0183.hpp
  src/response.cpp
  src/Response.hpp
  src/sentence.cpp
  src/Sentence.hpp
  src/talkerid.cpp
)

add_library(NMEA0183 STATIC ${SRC})
add_library(ocpn::nmea0183 ALIAS NMEA0183)
target_link_libraries(NMEA0183)

set_property(TARGET NMEA0183 PROPERTY COMPILE_FLAGS "${OBJ_VISIBILITY}")
target_include_directories(NMEA0183 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(NMEA0183 PRIVATE ${wxWidgets_INCLUDE_DIRS})
