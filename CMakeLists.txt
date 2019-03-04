##---------------------------------------------------------------------------
## Author:      Rasbats aka. Mike Rossiter
## Copyright:
## License:     wxWidgets License
##---------------------------------------------------------------------------

# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)

PROJECT(ShipDriver_pi)

SET(PACKAGE_NAME ShipDriver_pi)
SET(VERBOSE_NAME ShipDriver)
SET(TITLE_NAME ShipDriver)
SET(CPACK_PACKAGE_CONTACT "Mike Rossiter")

SET(VERSION_MAJOR "2")
SET(VERSION_MINOR "1")

SET(NAME_SUFFIX "ov50")

set (CMAKE_CXX_STANDARD 11)




#SET(CMAKE_BUILD_TYPE Debug)

OPTION(SHIPDRIVER_USE_SVG "Use SVG graphics" ON)

INCLUDE("cmake/PluginConfigure.cmake")

IF(SHIPDRIVER_USE_SVG)
  ADD_DEFINITIONS(-DSHIPDRIVER_USE_SVG)
ENDIF(SHIPDRIVER_USE_SVG)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAS_STD_CPP11_FLAG)
if(HAS_STD_CPP11_FLAG)
  add_compile_options(-std=c++11)
endif()

SET(SRC_ShipDriver
        src/ShipDriver_pi.h
        src/ShipDriver_pi.cpp
        src/icons.h
        src/icons.cpp
	    src/ShipDrivergui.h
	    src/ShipDrivergui.cpp
	    src/ShipDrivergui_impl.cpp
	    src/ShipDrivergui_impl.h
		src/ocpn_plugin.h
		src/folder.xpm
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

IF(WIN32)
SET(SRC_JSON
         src/wxJSON/jsonval.cpp
         src/wxJSON/jsonreader.cpp
         src/wxJSON/jsonwriter.cpp
         src/wx/json_defs.h
         src/wx/jsonval.h
         src/wx/jsonreader.h
         src/wx/jsonwriter.h)
ENDIF(WIN32)


ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_ShipDriver} ${SRC_JSON}
)

ADD_DEFINITIONS(-DTIXML_USE_STL)

INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")
