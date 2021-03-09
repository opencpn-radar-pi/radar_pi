cmake_minimum_required(VERSION 3.5.0)

if (TARGET radar::emulator)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
endif ()

SET(SRC
  include/emulator/emulatortype.h
  include/emulator/EmulatorControl.h
  include/emulator/EmulatorControlsDialog.h
  include/emulator/EmulatorControlSet.h
  include/emulator/EmulatorReceive.h

  src/EmulatorControl.cpp
  src/EmulatorControlsDialog.cpp
  src/EmulatorReceive.cpp
)

add_library(EMULATOR STATIC ${SRC})
add_library(radar::emulator ALIAS EMULATOR)

if (CMAKE_CX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET EMULATOR PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
target_link_libraries(EMULATOR PRIVATE
  radar::garminhd  radar::garminxhd radar::navico radar::raymarine ocpn::wxjson ocpn::nmea0183 ocpn::api
)

target_include_directories(EMULATOR PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(EMULATOR PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/emulator)
