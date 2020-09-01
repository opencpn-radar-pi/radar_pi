cmake_minimum_required(VERSION 3.1.0)

if (TARGET ocpn::tinyxml)
    return ()
endif ()

option(USE_SYSTEM_TINYXML "Use system copy of library if available" ON)

set(SRC
  src/tinyxml.cpp
  src/tinyxmlerror.cpp
  src/tinyxmlparser.cpp
  include/tinyxml.h
)

find_library(TINYXML_LIBRARY NAMES tinyxml)
find_path(TINYXML_INCLUDE_DIR NAMES tinyxml.h PATH_SUFFIXES tinyxml)

add_library(_TINYXML STATIC ${SRC})
add_library(ocpn::tinyxml ALIAS _TINYXML)

if (TINYXML_LIBRARY AND TINYXML_INCLUDE_DIR AND USE_SYSTEM_TINYXML)
  target_link_libraries(_TINYXML INTERFACE ${TINYXML_LIBRARY})
  target_include_directories(_TINYXML INTERFACE ${TINYXML_INCLUDE_DIR})
  message(STATUS "Using system tinyxml")
else ()
  target_compile_definitions(_TINYXML PUBLIC -DTIXML_USE_STL)
  target_include_directories(_TINYXML
                             PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  set_property(TARGET _TINYXML PROPERTY COMPILE_FLAGS "${OBJ_VISIBILITY}")
  message(STATUS "Using bundled tinyxml")
endif ()
