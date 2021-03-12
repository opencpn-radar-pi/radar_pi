cmake_minimum_required(VERSION 3.5.0)

if (TARGET radar::navico)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
endif ()

SET(SRC
  include/navico/Header.h
  include/navico/NavicoCommon.h
  include/navico/NavicoControl.h
  include/navico/NavicoControlsDialog.h
  include/navico/NavicoControlSet.h
  include/navico/NavicoLocate.h
  include/navico/br24type.h
  include/navico/br3gtype.h
  include/navico/br4gatype.h
  include/navico/br4gbtype.h
  include/navico/haloatype.h
  include/navico/halobtype.h
  include/navico/NavicoReceive.h

  src/NavicoControl.cpp
  src/NavicoControlsDialog.cpp
  src/NavicoLocate.cpp
  src/NavicoReceive.cpp
)

add_library(NAVICO STATIC ${SRC})
add_library(radar::navico ALIAS NAVICO)

if (CMAKE_CX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET NAVICO PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
target_link_libraries(NAVICO PRIVATE
  radar::garminhd radar::garminxhd radar::raymarine ocpn::wxjson ocpn::nmea0183 ocpn::api
)
target_include_directories(NAVICO PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(NAVICO
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/navico
)
