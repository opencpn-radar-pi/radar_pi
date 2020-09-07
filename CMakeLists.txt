# ~~~
# Author:      Rasbats aka. Mike Rossiter
# Copyright:
# License:     wxWidgets License
# ~~~

# -------- Cmake setup ---------
#
cmake_minimum_required(VERSION 3.5.1)

cmake_policy(SET CMP0042 NEW)

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif ()

# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")

# Locations where cmake looks for cmake modules.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/build
                      ${CMAKE_SOURCE_DIR}/
                      ${CMAKE_SOURCE_DIR}/cmake
)
if (WIN32)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/buildwin)
endif ()


# -------- Build setup (mostly in PluginCompiler) --------
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# -------- Options ----------
#
set(OCPN_UNSTABLE_REPO
    "mike-rossiter/shipdriver-beta"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_STABLE_REPO
    "mike-rossiter/shipdriver-prod"
    CACHE STRING "Default repository for tagged builds"
)
option(SHIPDRIVER_USE_SVG "Use SVG graphics" ON)

option(OCPN_FLATPAK "Build flatpak plugin" OFF)


# -------  Plugin setup --------
#
project(ShipDriver_pi VERSION 2.5.0)

set(VERBOSE_NAME ShipDriver)    # Legacy installers artifact names
set(TITLE_NAME ShipDriver)      # Legacy installer dialogs
set(PLUGIN_API_NAME ShipDriver) # As of GetCommonName() in plugin API
set(CPACK_PACKAGE_CONTACT "Mike Rossiter")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simulate ship movements")
set(CPACK_PACKAGE_DESCRIPTION "Simulate ship movements - or is it a game?")

set(OCPN_MIN_VERSION "ov50")

set(PKG_RELEASE "1")
set(PKG_API_VERSION "1.16")
set(PKG_AUTHOR "Mike Rossiter")

set(SRC
    src/ShipDriver_pi.h
    src/ShipDriver_pi.cpp
    src/icons.h
    src/icons.cpp
    src/ShipDrivergui.h
    src/ShipDrivergui.cpp
    src/ShipDrivergui_impl.cpp
    src/ShipDrivergui_impl.h
    src/AisMaker.h
    src/AisMaker.cpp
    src/GribRecord.cpp
    src/GribRecordSet.h
    src/GribRecord.h
)


# -------- Setup completed, build the plugin --------
#
set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(PACKAGE_NAME ${CMAKE_PROJECT_NAME})

set(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

if (SHIPDRIVER_USE_SVG)
  add_definitions(-DSHIPDRIVER_USE_SVG)
endif (SHIPDRIVER_USE_SVG)

add_library(${PACKAGE_NAME} SHARED ${SRC})
include_directories(${CMAKE_BINARY_DIR}/include)

if (OCPN_FLATPAK)
  include(Flatpak)
  include(Metadata)
  FlatpakTargets(
    ${PROJECT_SOURCE_DIR}/flatpak/org.opencpn.OpenCPN.Plugin.shipdriver.yaml
    ${pkg_tarname}.tar.gz
  )
else (OCPN_FLATPAK)
  include(PluginCompiler)
  include(PluginLibs)
  include(PluginInstall)
  include(PluginLocalization)
  include(PluginPackage)
  
  add_subdirectory("libs/api-16")
  target_link_libraries(${PACKAGE_NAME} ocpn::api)
  
  add_subdirectory("libs/jsoncpp")
  target_link_libraries(${PACKAGE_NAME} ocpn::jsoncpp)
  
  add_subdirectory("libs/tinyxml")
  target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)
  
  add_subdirectory("libs/wxJSON")
  target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)
endif (OCPN_FLATPAK)

configure_file(
  # The cloudsmith upload script
  ${CMAKE_SOURCE_DIR}/ci/upload.sh.in ${CMAKE_BINARY_DIR}/upload.sh
  @ONLY
)

configure_file(
  # The XML metadata file
  ${CMAKE_SOURCE_DIR}/shipdriver-plugin.xml.in
  ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml
  @ONLY
)
