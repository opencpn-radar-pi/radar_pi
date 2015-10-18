##---------------------------------------------------------------------------
## Plagiarized by: Dave Cowell for WINDOWS
##
## Author:      bdbcat aka. dsr (Dave Register)
## Copyright:
## License:     wxWidgets License
##---------------------------------------------------------------------------

# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)

PROJECT(br24radar_pi)

SET(PACKAGE_NAME br24radar_pi)
SET(VERBOSE_NAME BR24Radar)
SET(TITLE_NAME BR24Radar)
SET(CPACK_PACKAGE_CONTACT "kees@verruijt.net")

SET(VERSION_MAJOR "1")
SET(VERSION_MINOR "3")

#SET(CMAKE_BUILD_TYPE Debug)

INCLUDE("cmake/PluginConfigure.cmake")

# For convenience we define the sources as a variable. You can add
# header files and cpp/c files and CMake will sort them out

SET(SRC_br24radar
            src/br24radar_pi.h
            src/br24radar_pi.cpp
            src/shaderutil.h
            src/shaderutil.cpp
            src/br24radarcontrol.cpp
            src/br24messagebox.cpp
            src/GuardZoneControls.cpp
            src/GuardZoneBogey.cpp
            src/Idle_Dialog.cpp
)
SET(SRC_NMEA0183
        src/nmea0183/nmea0183.cpp
        src/nmea0183/nmea0183.hpp
        src/nmea0183/Response.hpp
        src/nmea0183/response.cpp
        src/nmea0183/Sentence.hpp
        src/nmea0183/sentence.cpp
        src/nmea0183/talkerid.cpp
        src/nmea0183/hexvalue.cpp
        src/nmea0183/expid.cpp
        src/nmea0183/hdt.hpp
        src/nmea0183/hdt.cpp
        src/nmea0183/hdg.hpp
        src/nmea0183/hdg.cpp
        src/nmea0183/hdm.hpp
        src/nmea0183/hdm.cpp
)
SET(SRC_JSON
        src/wxJSON/jsonreader.cpp
        src/wxJSON/jsonval.cpp
        # We don't use jsonwriter.cpp yet ...
)
INCLUDE_DIRECTORIES(src/nmea0183)
INCLUDE_DIRECTORIES(src/wxJSON)
INCLUDE_DIRECTORIES(src)

ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_br24radar} ${SRC_NMEA0183} ${SRC_JSON})

INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")
