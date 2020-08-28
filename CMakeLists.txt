# ~~~
# Author:      Rasbats aka. Mike Rossiter
# Copyright:
# License:     wxWidgets License
# ~~~

# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif ()

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif ()

# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")

project(ShipDriver_pi)

# Locations where cmake looks for cmake modules.
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/build ${CMAKE_SOURCE_DIR}/
                      ${CMAKE_SOURCE_DIR}/cmake
)

if (WIN32)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/buildwin)
endif ()

set(PACKAGE_NAME ShipDriver_pi)
set(VERBOSE_NAME ShipDriver)
set(PLUGIN_API_NAME ShipDriver) # As of GetCommonName() in plugin API
set(TITLE_NAME ShipDriver)
set(CPACK_PACKAGE_CONTACT "Mike Rossiter")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simulate ship movements")
set(CPACK_PACKAGE_DESCRIPTION "Simulate ship movements - or is it a game?")
set(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

set(VERSION_MAJOR "2")
set(VERSION_MINOR "4")
set(VERSION_PATCH "9")

set(VERSION_DATE "2020-05-09")
set(OCPN_MIN_VERSION "ov50")

set(CMAKE_CXX_STANDARD 11)

set(PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(PKG_RELEASE "1")
set(PKG_API_VERSION "1.16")
set(PKG_AUTHOR "Mike Rossiter")

# SET(CMAKE_BUILD_TYPE Debug)
set(wxWidgets_USE_DEBUG OFF)
set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_UNIVERSAL OFF)
set(wxWidgets_USE_STATIC OFF)

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

# cmake-format: off
# The cloudsmith upload script
function (configure_upload)
  configure_file(
    ${CMAKE_SOURCE_DIR}/ci/upload.sh.in ${CMAKE_BINARY_DIR}/upload.sh
    @ONLY
  )
endfunction (configure_upload)

# The XML metadata file
function (configure_metadata)
  configure_file(
    ${CMAKE_SOURCE_DIR}/shipdriver-plugin.xml.in
    ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml
    @ONLY
  )
endfunction (configure_metadata)
# cmake-format: on

if (SHIPDRIVER_USE_SVG)
  add_definitions(-DSHIPDRIVER_USE_SVG)
endif (SHIPDRIVER_USE_SVG)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAS_STD_CPP11_FLAG)
if (HAS_STD_CPP11_FLAG)
  add_compile_options(-std=c++11)
endif ()

set(SRC_ShipDriver
    api-16/ocpn_plugin.h
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

add_library(${PACKAGE_NAME} SHARED ${SRC_ShipDriver})
include_directories(${PROJECT_SOURCE_DIR}/api-16)
include_directories(${CMAKE_BINARY_DIR}/include)
add_definitions(-DTIXML_USE_STL)


if (OCPN_FLATPAK)
  include(GetArch)
  getarch()
  # Lie to PluginSetup that we have found wxWidgets so it don't barfs.
  set(wxWidgets_LIBRARIES FOOBAR)
  include(PluginSetup)
  unset(wxWidgets_LIBRARIES)
  include(metadata)

  configure_upload()
  configure_metadata()

  find_program(TAR NAMES gtar tar)
  if (NOT TAR)
    message(FATAL_ERROR "tar not found, required for OCPN_FLATPAK")
  endif ()
  set(FLATPAK_DIR ${PROJECT_SOURCE_DIR}/flatpak)
  add_custom_target(
    flatpak-build ALL
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND flatpak-builder --force-clean ${CMAKE_CURRENT_BINARY_DIR}/app
            ${FLATPAK_DIR}/org.opencpn.OpenCPN.Plugin.shipdriver.yaml
  )
  add_custom_target("flatpak-pkg")
  add_custom_command(
    TARGET flatpak-pkg
    COMMAND
      ${TAR} -czf ${pkg_tarname}.tar.gz --transform
      's|.*/files/|shipdriver-flatpak-${PACKAGE_VERSION}/|'
      ${CMAKE_CURRENT_BINARY_DIR}/app/files
  )
  return()
endif ()

include("GetArch")
getarch()
include("cmake/PluginConfigure.cmake")
include("cmake/PluginInstall.cmake")
include("cmake/PluginSetup.cmake")
include("cmake/PluginLocalization.cmake")
include("cmake/metadata.cmake")
include("cmake/PluginPackage.cmake")

add_subdirectory("libs/jsoncpp")
target_link_libraries(${PACKAGE_NAME} ocpn::jsoncpp)

add_subdirectory("libs/tinyxml")
target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)

add_subdirectory("libs/wxJSON")
target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

configure_metadata()
configure_upload()
