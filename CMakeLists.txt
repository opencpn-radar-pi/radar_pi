##---------------------------------------------------------------------------
## Author:      Rasbats aka. Mike Rossiter
## Copyright:
## License:     wxWidgets License
##---------------------------------------------------------------------------

# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif ()

# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")


PROJECT(ShipDriver_pi)

# Locations where cmake looks for cmake modules.
set(
  CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/build
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_SOURCE_DIR}/cmake
)

if (WIN32)
  SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/buildwin)
endif ()


SET(PACKAGE_NAME ShipDriver_pi)
SET(VERBOSE_NAME ShipDriver)
SET(PLUGIN_API_NAME ShipDriver)         # As of GetCommonName() in plugin API
SET(TITLE_NAME ShipDriver)
SET(CPACK_PACKAGE_CONTACT "Mike Rossiter")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simulate ship movements")
SET(CPACK_PACKAGE_DESCRIPTION "Simulate ship movements - or is it a game?")
SET(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

SET(VERSION_MAJOR "2")
SET(VERSION_MINOR "4")
SET(VERSION_PATCH "1")

SET(VERSION_DATE "2020-05-09")
SET(OCPN_MIN_VERSION "ov50")

set (CMAKE_CXX_STANDARD 11)

SET(PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
SET(PKG_RELEASE "1")
SET(PKG_API_VERSION "1.16")
SET(PKG_AUTHOR "Mike Rossiter")
SET(PKG_NVR ${PACKAGE_NAME}-${PACKAGE_VERSION}-${PKG_RELEASE})

#SET(CMAKE_BUILD_TYPE Debug)
SET( wxWidgets_USE_DEBUG OFF)
SET( wxWidgets_USE_UNICODE ON)
SET( wxWidgets_USE_UNIVERSAL OFF)
SET( wxWidgets_USE_STATIC OFF)


SET(OCPN_UNSTABLE_REPO "david-register/ocpn-plugins-unstable"
    CACHE STRING "Default repository for untagged builds")
SET(OCPN_STABLE_REPO "david-register/ocpn-plugins-stable"
    CACHE STRING "Default repository for tagged builds")
OPTION(SHIPDRIVER_USE_SVG "Use SVG graphics" ON)
# For OpenCPN version 5.x, NOT 5.0 ON. For version 5.0 OFF
OPTION(MANAGED_PLUGIN "Use managed plugin" OFF)
OPTION(OCPN_FLATPAK "Build flatpak plugin" OFF)


INCLUDE("cmake/PluginJSON.cmake")
# The cloudsmith upload script
function (configure_upload)
  configure_file(
    ${CMAKE_SOURCE_DIR}/ci/upload.sh.in
    ${CMAKE_BINARY_DIR}/upload.sh
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

IF(SHIPDRIVER_USE_SVG)
  ADD_DEFINITIONS(-DSHIPDRIVER_USE_SVG)
ENDIF(SHIPDRIVER_USE_SVG)

IF(MANAGED_PLUGIN)
ADD_DEFINITIONS(-DMANAGED_PLUGIN)
ENDIF(MANAGED_PLUGIN)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAS_STD_CPP11_FLAG)
if(HAS_STD_CPP11_FLAG)
  add_compile_options(-std=c++11)
endif()


SET(SRC_ShipDriver
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
	    src/tinyxml.h
		src/tinyxml.cpp
	    src/tinyxmlerror.cpp
		src/tinyxmlparser.cpp		
	)


ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_ShipDriver} ${SRC_JSON}
)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/api-16)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
ADD_DEFINITIONS(-DTIXML_USE_STL)

if (OCPN_FLATPAK)
  include(GetArch)
  GetArch()
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
  set (FLATPAK_DIR ${PROJECT_SOURCE_DIR}/flatpak)
  add_custom_target(flatpak-build ALL
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMAND flatpak-builder --force-clean
          ${CMAKE_CURRENT_BINARY_DIR}/app
          ${FLATPAK_DIR}/org.opencpn.OpenCPN.Plugin.shipdriver.yaml
  )
  add_custom_target("flatpak-pkg")
  add_custom_command(
      TARGET flatpak-pkg
      COMMAND ${TAR}
          -czf ${pkg_tarname}.tar.gz
          --transform 's|.*/files/|shipdriver-flatpak-${PACKAGE_VERSION}/|'
          ${CMAKE_CURRENT_BINARY_DIR}/app/files
  )
  return ()
endif()

INCLUDE("GetArch")
GetArch()
INCLUDE("cmake/PluginConfigure.cmake")
INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginSetup.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/metadata.cmake")
INCLUDE("cmake/PluginPackage.cmake")

configure_metadata()
configure_upload()
