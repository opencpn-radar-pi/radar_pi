##---------------------------------------------------------------------------
## Plagiarized by: Dave Cowell for WINDOWS
##
## Author:      bdbcat aka. dsr (Dave Register)
## Copyright:
## License:     wxWidgets License
##---------------------------------------------------------------------------

# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)

PROJECT(radar_pi)

SET(PACKAGE_NAME radar_pi)
SET(VERBOSE_NAME Radar)
SET(TITLE_NAME Radar)
SET(CPACK_PACKAGE_CONTACT "kees@verruijt.net")
SET(VERSION_MAJOR "4")
#SET(CMAKE_BUILD_TYPE Debug)
SET(VERSION_MINOR "927")
SET(VERSION_PATCH "0")
SET(VERSION_DATE "2017-09-27")

INCLUDE("cmake/PluginConfigure.cmake")

# For convenience we define the sources as a variable. You can add
# header files and cpp/c files and CMake will sort them out

SET(SRC_radar
            src/ControlsDialog.cpp
            src/ControlsDialog.h
            src/GuardZone.cpp
            src/GuardZone.h
            src/GuardZoneBogey.cpp
            src/GuardZoneBogey.h
            src/Kalman.cpp
            src/Kalman.h
            src/Matrix.h
            src/MessageBox.cpp
            src/MessageBox.h
            src/NavicoControlsDialog.cpp
            src/NavicoControlsDialog.h
            src/OptionsDialog.cpp
            src/OptionsDialog.h
            src/RadarCanvas.cpp
            src/RadarCanvas.h
            src/RadarControl.h
            src/RadarDraw.cpp
            src/RadarDraw.h
            src/RadarDrawShader.cpp
            src/RadarDrawShader.h
            src/RadarDrawVertex.cpp
            src/RadarDrawVertex.h
            src/RadarInfo.cpp
            src/RadarInfo.h
            src/RadarMarpa.cpp
            src/RadarMarpa.h
            src/RadarPanel.cpp
            src/RadarPanel.h
            src/RadarReceive.h
            src/TextureFont.cpp
            src/TextureFont.h
            src/NavicoReceive.cpp
            src/NavicoReceive.h
            src/br24Transmit.cpp
            src/br24Transmit.h
            src/drawutil.cpp
            src/drawutil.h
            src/icons.cpp
            src/icons.h
            src/pi_common.h
            src/radar_pi.cpp
            src/radar_pi.h
            src/shaderutil.cpp
            src/shaderutil.h
            src/socketutil.cpp
            src/socketutil.h

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


IF(WIN32)
  SET(wxWidgets_USE_LIBS base core net adv gl aui)
  SET(BUILD_SHARED_LIBS TRUE)
  FIND_PACKAGE(wxWidgets REQUIRED)
ENDIF(WIN32)

ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_radar} ${SRC_NMEA0183} ${SRC_JSON})

SET(TEST_KALMAN kalman-test)
SET(SRC_KALMAN
              src/Kalman-test.cpp
              src/Kalman.h
              src/Kalman.cpp
              src/Matrix.h
              src/RadarMarpa.h
)
ADD_EXECUTABLE(${TEST_KALMAN} ${SRC_KALMAN})
TARGET_LINK_LIBRARIES(${TEST_KALMAN} ${wxWidgets_LIBRARIES})

INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")
