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
SET(VERSION_MAJOR "3")
#SET(CMAKE_BUILD_TYPE Debug)
SET(VERSION_MINOR "0")
SET(VERSION_PATCH "0")
SET(VERSION_DATE "2017-09-24")

INCLUDE("cmake/PluginConfigure.cmake")

# For convenience we define the sources as a variable. You can add
# header files and cpp/c files and CMake will sort them out

SET(SRC_br24radar
            src/pi_common.h
            src/shaderutil.h
            src/shaderutil.cpp
            src/socketutil.h
            src/socketutil.cpp
            src/drawutil.h
            src/drawutil.cpp
            src/br24radar_pi.h
            src/br24radar_pi.cpp
            src/br24ControlsDialog.h
            src/br24ControlsDialog.cpp
            src/br24MessageBox.h
            src/br24MessageBox.cpp
            src/br24OptionsDialog.h
            src/br24OptionsDialog.cpp
            src/br24Receive.h
            src/br24Receive.cpp
            src/RadarMarpa.h
            src/RadarMarpa.cpp
            src/br24Transmit.h
            src/br24Transmit.cpp
            src/icons.h
            src/icons.cpp
            src/GuardZone.h
            src/GuardZone.cpp
            src/GuardZoneBogey.h
            src/GuardZoneBogey.cpp
            src/Kalman.h
            src/Kalman.cpp
            src/Matrix.h
            src/RadarInfo.h
            src/RadarInfo.cpp
            src/RadarCanvas.h
            src/RadarCanvas.cpp
            src/RadarPanel.h
            src/RadarPanel.cpp
            src/RadarDraw.h
            src/RadarDraw.cpp
            src/RadarDrawShader.h
            src/RadarDrawShader.cpp
            src/RadarDrawVertex.h
            src/RadarDrawVertex.cpp
            src/TextureFont.h
            src/TextureFont.cpp
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

ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_br24radar} ${SRC_NMEA0183} ${SRC_JSON})

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
