cmake_minimum_required(VERSION 3.5.0)

if (TARGET radar::raymarine)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
endif ()

SET(SRC
  include/raymarine/RME120type.h
  include/raymarine/RME120ControlSet.h
  include/raymarine/RaymarineCommon.h
  include/raymarine/RME120Control.h
  include/raymarine/RME120ControlsDialog.h
  include/raymarine/RME120Receive.h

  src/RaymarineLocate.cpp
  src/RME120Control.cpp
  src/RME120ControlsDialog.cpp
  src/RME120Receive.cpp
)

add_library(RAYMARINE STATIC ${SRC})
add_library(radar::raymarine ALIAS RAYMARINE)

if (CMAKE_CX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET RAYMARINE PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
target_link_libraries(RAYMARINE
  PRIVATE radar::garminhd radar::garminxhd radar::navico ocpn::wxjson ocpn::nmea0183 ocpn::api
)
target_include_directories(RAYMARINE PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(RAYMARINE
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/raymarine
)
