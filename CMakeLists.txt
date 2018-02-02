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
SET(VERSION_MINOR "4")

#SET(CMAKE_BUILD_TYPE Debug)
SET( wxWidgets_USE_DEBUG OFF)
SET( wxWidgets_USE_UNICODE ON)
SET( wxWidgets_USE_UNIVERSAL OFF)
SET( wxWidgets_USE_STATIC OFF)

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
	)


ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_ShipDriver})

ADD_DEFINITIONS(-DTIXML_USE_STL)

TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${wxWidgets_LIBRARIES} )

IF(WIN32)
	INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
	SET(INSTALL_DIRECTORY "plugins\\\\${PACKAGE_NAME}")
ENDIF(WIN32)


INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")
