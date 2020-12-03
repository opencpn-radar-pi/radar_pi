#
# Export variables used in plugin setup: GIT_HASH, GIT_COMMIT, GIT_TAG,
# PKG_TARGET  and PKG_TARGET_VERSION.

if (DEFINED PKG_TARGET)
  return ()
endif ()

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git tag --contains HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if ("${BUILD_TYPE}" STREQUAL "flatpak")
  set(PKG_TARGET "flatpak")
  set(PKG_TARGET_VERSION "18.08") # As of flatpak/*yaml
elseif (MINGW)
  set(PKG_TARGET "mingw")
  if (CMAKE_SYSTEM_VERSION)
    set(PKG_TARGET_VERSION ${CMAKE_SYSTEM_VERSION})
  else ()
    set(PKG_TARGET_VERSION 10)
  endif ()
elseif (MSVC)
  set(PKG_TARGET "msvc")
  if (CMAKE_SYSTEM_VERSION)
    set(PKG_TARGET_VERSION ${CMAKE_SYSTEM_VERSION})
  elseif (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(PKG_TARGET_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
  else ()
    set(PKG_TARGET_VERSION 10)
  endif ()
elseif (APPLE)
  set(PKG_TARGET "darwin")
  set(PKG_TARGET_VERSION "10.13.6")
elseif (UNIX)
  # Some linux dist:
  execute_process(
    COMMAND "lsb_release" "-is"
    OUTPUT_VARIABLE PKG_TARGET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND "lsb_release" "-rs"
    OUTPUT_VARIABLE PKG_TARGET_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else ()
  set(PKG_TARGET "unknown")
  set(PKG_TARGET_VERSION 1)
endif ()

string(STRIP "${PKG_TARGET}" PKG_TARGET)
string(TOLOWER "${PKG_TARGET}" PKG_TARGET)
string(STRIP "${PKG_TARGET_VERSION}" PKG_TARGET_VERSION)
string(TOLOWER "${PKG_TARGET_VERSION}" PKG_TARGET_VERSION)

if (PKG_TARGET STREQUAL "ubuntu")
  if (DEFINED wxWidgets_CONFIG_EXECUTABLE)
    set(_WX_CONFIG_PROG ${wxWidgets_CONFIG_EXECUTABLE})
  else ()
    find_program(_WX_CONFIG_PROG NAMES $ENV{WX_CONFIG} wx-config )
  endif ()
  if (_WX_CONFIG_PROG)
    execute_process(
      COMMAND ${_WX_CONFIG_PROG} --selected-config
      OUTPUT_VARIABLE _WX_SELECTED_CONFIG
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (_WX_SELECTED_CONFIG MATCHES gtk3)
      set(PKG_TARGET ubuntu-gtk3)
    endif ()
  else ()
    message(WARNING "Cannot locate wx-config utility")
  endif ()
endif ()
