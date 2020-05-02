##---------------------------------------------------------------------------
## Author:      Sean D'Epagnier
## Copyright:   2015
## License:     GPLv3+
##---------------------------------------------------------------------------

SET(SRC_JSON
	    src/jsoncpp/json_reader.cpp
	    src/jsoncpp/json_value.cpp
	    src/jsoncpp/json_writer.cpp
            )

IF(QT_ANDROID)
  ADD_DEFINITIONS(-DJSONCPP_NO_LOCALE_SUPPORT)
ENDIF(QT_ANDROID)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/jsoncpp)
