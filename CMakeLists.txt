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

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "5")

#SET(CMAKE_BUILD_TYPE Debug)

OPTION(SHIPDRIVER_USE_SVG "Use SVG graphics" ON)

INCLUDE("cmake/PluginConfigure.cmake")

IF(SHIPDRIVER_USE_SVG)
  ADD_DEFINITIONS(-DSHIPDRIVER_USE_SVG)
ENDIF(SHIPDRIVER_USE_SVG)

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
	)


ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_ShipDriver})

INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")
