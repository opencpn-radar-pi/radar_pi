# ~~~
# Author:      Sean D'Epagnier
# Copyright:   2015
# License:     GPLv3+
# ~~~

set(SRC_JSON 
    src/jsoncpp/json_reader.cpp
    src/jsoncpp/json_value.cpp
    src/jsoncpp/json_writer.cpp
)

if (QT_ANDROID)
  add_definitions(-DJSONCPP_NO_LOCALE_SUPPORT)
endif (QT_ANDROID)

include_directories(${CMAKE_SOURCE_DIR}/src/jsoncpp)
