# ~~~
# Summary:      Plugin-specific build setup
# Summary:      Plugin-specific build setup
# Copyright (c) 2020-2021 Mike Rossiter
#               2021 Alec Leamas
# License:      GPLv3+
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.



# -------- Options ----------

set(OCPN_TEST_REPO
    "opencpn-radar-pi/opencpn-radar-pi-unstable"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "opencpn-radar-pi/opencpn-radar-pi-beta"
    CACHE STRING 
    "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "opencpn-radar-pi/opencpn-radar-pi-prod"
    CACHE STRING 
    "Default repository for tagged builds not matching 'beta'"
)

#
# -------  Plugin setup --------
#
set(PKG_NAME radar_pi)
set(PKG_VERSION 5.4.1)
set(PKG_PRERELEASE "alpha")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME radar)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME Radar) # As of GetCommonName() in plugin API
set(CPACK_PACKAGE_CONTACT "kees@verruijt.net")   ##  Not used
set(PKG_SUMMARY "Overlays the radar picture on OpenCPN")
set(PKG_DESCRIPTION [=[
Garmin, Navico and Raymarine radar support

WARNING: OPENGL MODE IS REQUIRED!

Works with Garmin HD, xHD, Navico BR24, 3G, 4G, HALOxx and several Raymarine radars.

When a compass heading is provided it will allow radar overlay on the chart(s).
It also allows separate display of a traditional radar picture (PPI).

Supports MARPA (even on radars that do not support this themselves),
Guard zones, AIS overlay on PPI, and various radar dependent features
such as dual radar range and Doppler.
]=])

set(PKG_AUTHOR "Hakan Svensson / Douwe Fokkema / Kees Verruijt / David S Register")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE https://github.com/opencpn-radar-pi/radar_pi)
set(PKG_INFO_URL https://opencpn.org/OpenCPN/plugins/radarPI.html)


set(SRC
  include/ControlsDialog.h
  include/GuardZone.h
  include/GuardZoneBogey.h
  include/Kalman.h
  include/Matrix.h
  include/MessageBox.h
  include/OptionsDialog.h
  include/RadarCanvas.h
  include/RadarControl.h
  include/RadarControlItem.h
  include/RadarDraw.h
  include/RadarDrawShader.h
  include/RadarDrawVertex.h
  include/RadarFactory.h
  include/RadarInfo.h
  include/RadarLocationInfo.h
  include/RadarMarpa.h
  include/RadarPanel.h
  include/RadarReceive.h
  include/RadarType.h
  include/SelectDialog.h
  include/SoftwareControlSet.h
  include/TextureFont.h
  include/TrailBuffer.h
  include/drawutil.h
  include/icons.h
  include/pi_common.h
  include/radar_pi.h
  include/shaderutil.h
  include/socketutil.h

  # Source files that are repeatedly included to get a 
  # different effect every time
  include/ControlType.inc
  include/shaderutil.inc

  # Headers for radar specific files

  include/emulator/EmulatorControl.h
  include/emulator/EmulatorControlSet.h
  include/emulator/EmulatorControlsDialog.h
  include/emulator/EmulatorReceive.h
  include/emulator/emulatortype.h
  include/garminhd/GarminHDControl.h
  include/garminhd/GarminHDControlSet.h
  include/garminhd/GarminHDControlsDialog.h
  include/garminhd/GarminHDReceive.h
  include/garminhd/garminhdtype.h
  include/garminxhd/GarminxHDControl.h
  include/garminxhd/GarminxHDControlSet.h
  include/garminxhd/GarminxHDControlsDialog.h
  include/garminxhd/GarminxHDReceive.h
  include/garminxhd/garminxhdtype.h
  include/navico/Header.h
  include/navico/NavicoCommon.h
  include/navico/NavicoControl.h
  include/navico/NavicoControlSet.h
  include/navico/NavicoControlsDialog.h
  include/navico/NavicoLocate.h
  include/navico/NavicoReceive.h
  include/navico/br24type.h
  include/navico/br3gtype.h
  include/navico/br4gatype.h
  include/navico/br4gbtype.h
  include/navico/haloatype.h
  include/navico/halobtype.h
  include/raymarine/RME120Control.h
  include/raymarine/RMQuantumControl.h
  include/raymarine/RME120ControlSet.h
  include/raymarine/RME120ControlsDialog.h
  include/raymarine/RaymarineReceive.h
  include/raymarine/RME120type.h
  include/raymarine/RMQuantumtype.h
  include/raymarine/RaymarineCommon.h
  include/raymarine/RaymarineLocate.h
  include/raymarine/RMQuantumControlsDialog.h
  include/raymarine/RMQuantumControl.h
  include/raymarine/RMQuantumControlSet.h

  src/ControlsDialog.cpp
  src/GuardZone.cpp
  src/GuardZoneBogey.cpp
  src/Kalman.cpp
  src/MessageBox.cpp
  src/OptionsDialog.cpp
  src/RadarCanvas.cpp
  src/RadarDraw.cpp
  src/RadarDrawShader.cpp
  src/RadarDrawVertex.cpp
  src/RadarFactory.cpp
  src/RadarInfo.cpp
  src/RadarMarpa.cpp
  src/RadarPanel.cpp
  src/SelectDialog.cpp
  src/TextureFont.cpp
  src/TrailBuffer.cpp
  src/drawutil.cpp
  src/icons.cpp
  src/radar_pi.cpp
  src/shaderutil.cpp
  src/socketutil.cpp

  src/emulator/EmulatorControl.cpp
  src/emulator/EmulatorControlsDialog.cpp
  src/emulator/EmulatorReceive.cpp
  src/garminhd/GarminHDControl.cpp
  src/garminhd/GarminHDControlsDialog.cpp
  src/garminhd/GarminHDReceive.cpp
  src/garminxhd/GarminxHDControl.cpp
  src/garminxhd/GarminxHDControlsDialog.cpp
  src/garminxhd/GarminxHDReceive.cpp
  src/navico/NavicoControl.cpp
  src/navico/NavicoControlsDialog.cpp
  src/navico/NavicoLocate.cpp
  src/navico/NavicoReceive.cpp
  src/raymarine/RME120Control.cpp
  src/raymarine/RMQuantumControl.cpp
  src/raymarine/RME120ControlsDialog.cpp
  src/raymarine/RaymarineReceive.cpp
  src/raymarine/RaymarineLocate.cpp
  src/raymarine/RMQuantumControlsDialog.cpp
)

set(PKG_API_LIB api-16)  #  A directory in libs/ e. g., api-17 or api-16

macro(late_init)
  # Perform initialization after the PACKAGE_NAME library, compilers
  # and ocpn::api is available.

  # Fix OpenGL deprecated warnings in Xcode
  target_compile_definitions(${PACKAGE_NAME} PRIVATE GL_SILENCE_DEPRECATION)

  target_include_directories(${PACKAGE_NAME} PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/include 
    ${CMAKE_CURRENT_LIST_DIR}/include/emulator
    ${CMAKE_CURRENT_LIST_DIR}/include/garminhd
    ${CMAKE_CURRENT_LIST_DIR}/include/garminxhd
    ${CMAKE_CURRENT_LIST_DIR}/include/navico
    ${CMAKE_CURRENT_LIST_DIR}/include/raymarine
  )
endmacro ()

macro(add_plugin_libraries)
  # Add libraries required by this plugin

  add_subdirectory("libs/nmea0183")
  target_link_libraries(${PACKAGE_NAME} ocpn::nmea0183)

  add_subdirectory("opencpn-libs/wxJSON")
  target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)
endmacro ()
