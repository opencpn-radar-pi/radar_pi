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
SET(TITLE_NAME ShipDriver)
SET(CPACK_PACKAGE_CONTACT "Mike Rossiter")

SET(VERSION_MAJOR "2")
SET(VERSION_MINOR "4")

SET(VERSION_DATE "2020-05-09")
SET(OCPN_MIN_VERSION "ov50")

set (CMAKE_CXX_STANDARD 11)

#SET(CMAKE_BUILD_TYPE Debug)
SET( wxWidgets_USE_DEBUG OFF)
SET( wxWidgets_USE_UNICODE ON)
SET( wxWidgets_USE_UNIVERSAL OFF)
SET( wxWidgets_USE_STATIC OFF)


OPTION(SHIPDRIVER_USE_SVG "Use SVG graphics" ON)
# For OpenCPN version 5.x, NOT 5.0 ON. For version 5.0 OFF
OPTION(MANAGED_PLUGIN "Use managed plugin" OFF)


INCLUDE("cmake/PluginConfigure.cmake")
INCLUDE("cmake/PluginJSON.cmake")

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
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/include")
ADD_DEFINITIONS(-DTIXML_USE_STL)

INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")
