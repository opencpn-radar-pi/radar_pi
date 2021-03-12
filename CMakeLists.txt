# ~~~
# Author:      Rasbats aka. Mike Rossiter
# Copyright:
# License:     wxWidgets License
# ~~~

# -------- Cmake setup ---------
#
cmake_minimum_required(VERSION 3.12.0)

cmake_policy(SET CMP0042 NEW)

if (POLICY CMP0053)
    CMAKE_POLICY(SET CMP0053 NEW)
endif ()

if (POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif ()

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif ()

# Locations where cmake looks for cmake modules.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/build ${CMAKE_SOURCE_DIR}/cmake)

if (WIN32)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/buildwin)
endif ()
message(STATUS "Cmake version: ${CMAKE_VERSION}.")


# -------- Build setup (mostly in PluginCompiler) --------
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
message(STATUS "Building: ${BUILD_TYPE}")


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
project(radar_pi VERSION 5.2.0.1)
set(PKG_RELEASE "1")
set(PKG_PRERELEASE "beta1")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME radar)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME Radar) # As of GetCommonName() in plugin API
SET(CPACK_PACKAGE_CONTACT "kees@verruijt.net")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Overlays the radar picture on OpenCPN")
set(CPACK_PACKAGE_DESCRIPTION [=[
Garmin, Navico and Raymarine radar support

WARNING: OPENGL MODE IS REQUIRED!

Works with Garmin HD, xHD, Navico BR24, 3G, 4G, HALOxx and older Raymarine radars.

When a compass heading is provided it will allow radar overlay on the chart(s).
It also allows separate display of a traditional radar picture (PPI).

Supports MARPA (even on radars that do not support this themselves),
Guard zones, AIS overlay on PPI, and various radar dependent features
such as dual radar range and Doppler.
]=])

set(PKG_AUTHOR "Hakan Svensson / Douwe Fokkema / Kees Verruijt / David S Register")
set(PKG_IS_OPEN_SOURCE "yes")
set(CPACK_PACKAGE_HOMEPAGE_URL https://github.com/opencpn-radar-pi/radar_pi)
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

)


# -------- Setup completed, build the plugin --------
#
add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC})
include_directories(BEFORE ${CMAKE_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory("libs/api-16")
target_link_libraries(${CMAKE_PROJECT_NAME} ocpn::api)

set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(PACKAGE_NAME ${CMAKE_PROJECT_NAME})

# Create the minimim ocpn version id string like ov50
string(REGEX REPLACE "\([0-9]\)\.\([0-9]\).*" "ov\\1\\2"
       OCPN_MIN_VERSION ${API_OCPN_MIN_VERSION})

# Set up targets. Targets sets up a recursive call with BUILD_TYPE set to
# 'flatpak', 'pkg' or 'tarball'. The initial call without BUILD_TYPE ends
# here.
#
include(Targets)
create_targets(
  ${PROJECT_SOURCE_DIR}/flatpak/org.opencpn.OpenCPN.Plugin.radar.yaml
)
if ("${BUILD_TYPE}" STREQUAL "")
  return ()
endif ()

if (NOT ${BUILD_TYPE} STREQUAL "flatpak")
  # Build package as required (flatpak already dealt with).
  #
  include(PluginCompiler)
  include(PluginLibs)
  include(PluginLocalization)
  include(PluginInstall)
  include(PluginPackage)

  add_subdirectory("libs/nmea0183")
  target_link_libraries(${PACKAGE_NAME} ocpn::nmea0183)
  
  add_subdirectory("libs/wxJSON")
  target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

  #
  # Radar specific directories
  #
  add_subdirectory("src/emulator")
  target_link_libraries(${PACKAGE_NAME} radar::emulator)

  add_subdirectory("src/garminhd")
  target_link_libraries(${PACKAGE_NAME} radar::garminhd)

  add_subdirectory("src/garminxhd")
  target_link_libraries(${PACKAGE_NAME} radar::garminxhd)

  add_subdirectory("src/navico")
  target_link_libraries(${PACKAGE_NAME} radar::navico)

  add_subdirectory("src/raymarine")
  target_link_libraries(${PACKAGE_NAME} radar::raymarine)

endif ()

configure_file(
  ${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/include//config.h
  @ONLY
)

configure_file(
  # The cloudsmith upload script
  ${CMAKE_SOURCE_DIR}/ci/upload.sh.in ${CMAKE_BINARY_DIR}/upload.sh
  @ONLY
)
configure_file(
  # The cloudsmith upload script, windows bat file.
  ${CMAKE_SOURCE_DIR}/ci/upload.bat.in ${CMAKE_BINARY_DIR}/upload.bat
  @ONLY
)

set(checksum "@checksum@")
configure_file(
  # The XML metadata file
  ${CMAKE_SOURCE_DIR}/radar-plugin.xml.in
  ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml.in
  @ONLY
)
