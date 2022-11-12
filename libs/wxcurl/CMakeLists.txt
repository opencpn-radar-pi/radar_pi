cmake_minimum_required(VERSION 3.1)

if (TARGET ocpn::wxcurl)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
endif ()

# Library is used also in the plugins, so:
set(CMAKE_POSITION_INDEPENDENT_CODE "ON")

add_library(WXCURL_IF INTERFACE)
add_library(ocpn::wxcurl ALIAS WXCURL_IF)

if (QT_ANDROID)
  # Return an empty, no-op library -- assumes __OCPN_USE_CURL__ is undef.
  return ()
endif ()

if (WIN32 OR MINGW)
  option(OCPN_USE_PREBUILT_WXCURL "Use wxCurl libraries" ON)
else ()
  set(OCPN_USE_PREBUILT_WXCURL OFF)
endif ()

if (OCPN_USE_PREBUILT_WXCURL)
  set(_implib ${CMAKE_CURRENT_LIST_DIR}/win/libcurl.dll)
  add_library(CURL_LIBRARIES STATIC IMPORTED)
  set_property(TARGET CURL_LIBRARIES
    PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/win/libcurl.lib
  )
  set(CURL_LIBRARIES ${CMAKE_CURRENT_LIST_DIR}/win/libcurl.lib)
  set(CURL_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
else ()
  find_package(CURL REQUIRED)
  target_link_libraries(WXCURL_IF INTERFACE CURL::libcurl)
endif ()

set(SRC
  include/README
  include/curl
  include/curl/curlbuild.h
  include/curl/curlrules.h
  include/curl/easy.h
  include/curl/curl.h
  include/curl/curlbuild.h.in
  include/curl/curlbuild.h.cmake
  include/curl/typecheck-gcc.h
  include/curl/curlver.h
  include/curl/mprintf.h
  include/curl/stdcheaders.h
  include/curl/multi.h
  src/wx/curl
  src/wx/curl/utils.h
  src/wx/curl/http.h
  src/wx/curl/ftp.h
  src/wx/curl/ftptool.h
  src/wx/curl/dialog.h
  src/wx/curl/thread.h
  src/wx/curl/panel.h
  src/wx/curl/base.h
  src/wx/curl/telnet.h
  src/wx/curl/davtool.h
  src/wx/curl/dav.h
  src/utils.cpp
  src/davtool.cpp
  src/ftpparse.cpp
  src/panel.cpp
  src/ftpparse.h
  src/telnet.cpp
  src/ftp.cpp
  src/base.cpp
  src/dialog.cpp
  src/http.cpp
  src/thread.cpp
  src/ftptool.cpp
  src/dav.cpp
)

add_library(WXCURL_SRC STATIC ${SRC})
target_include_directories(WXCURL_SRC
  PUBLIC ${CURL_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/src
)
target_include_directories(WXCURL_IF
  INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src
)
target_link_libraries(WXCURL_SRC PUBLIC ${CURL_LIBRARIES})

if (WIN32)
  target_link_libraries(WXCURL_IF INTERFACE WXCURL_SRC)
endif (WIN32)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set_property(TARGET WXCURL_SRC PROPERTY COMPILE_FLAGS "-fvisibility=hidden")
endif ()
