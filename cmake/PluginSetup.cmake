#
#  Export variables used in plugin setup: GIT_HASH, GIT_COMMIT,
#  PKG_TARGET, PKG_TARGET_VERSION and PKG_NVR
#
#  Modified from the oesenc version. This one is independent of the 
#  cmake/* files and can be run first, it generates the
#  CPACK_PACKAGE_FILE_NAME for all builds; ci and normal.

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git log -1 --format=%ci
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (OCPN_TARGET AND OCPN_TARGET_ARCH AND OCPN_TARGET_VERSION)
    set(PKG_TARGET ${OCPN_TARGET}-${OCPN_TARGET_ARCH})
    set(PKG_TARGET_ARCH ${OCPN_TARGET_ARCH})
    set(PKG_TARGET_VERSION ${OCPN_TARGET_VERSION})
elseif (OCPN_FLATPAK)
    # See Opencpn bug #2003
    set(PKG_TARGET "flatpak-x86_64")
    set(PKG_TARGET_ARCH "x86_64")
    set(PKG_TARGET_VERSION "18.08")    # As of flatpak/*yaml
elseif (MINGW)
    set(PKG_TARGET "mingw")
    set(PKG_TARGET_ARCH "x86_64")
    if (CMAKE_SYSTEM_VERSION)
        set(PKG_TARGET_VERSION ${CMAKE_SYSTEM_VERSION})
    else ()
        set(PKG_TARGET_VERSION 10)
    endif ()
elseif (MSVC)
    set(PKG_TARGET "msvc")
    set(PKG_TARGET_ARCH "x86_64")
    if (CMAKE_SYSTEM_VERSION)
        set(PKG_TARGET_VERSION ${CMAKE_SYSTEM_VERSION})
    elseif (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
        set(PKG_TARGET_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
    else ()
        set(PKG_TARGET_VERSION 10)
    endif ()
elseif (APPLE)
    set(PKG_TARGET "darwin")
    set(PKG_TARGET_ARCH "x86_64")
    execute_process(COMMAND "sw_vers" "-productVersion"
                    OUTPUT_VARIABLE PKG_TARGET_VERSION)
elseif (UNIX)
    # Some linux dist:
    execute_process(COMMAND "lsb_release" "-is"
                    OUTPUT_VARIABLE PKG_TARGET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "lsb_release" "-rs"
                    OUTPUT_VARIABLE PKG_TARGET_VERSION
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(TOLOWER "${PKG_TARGET}" PKG_TARGET)

    # Handle gtk3 build variant
    if (NOT DEFINED wxWidgets_LIBRARIES)
        message(FATAL_ERROR "PluginSetup: required wxWidgets_LIBRARIES missing")
    elseif ("${wxWidgets_LIBRARIES}" MATCHES "gtk3u" AND PKG_TARGET MATCHES "ubuntu.*")
        set(PKG_TARGET "${PKG_TARGET}-gtk3")
    endif ()

    IF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
      SET (PKG_TARGET_ARCH "arm64")
    ELSEIF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
      SET (PKG_TARGET_ARCH "armhf")
    ELSEIF (PKG_TARGET MATCHES "raspbian")
      SET (PKG_TARGET_ARCH "armhf")
    ELSE ()
      IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (PKG_TARGET_ARCH "x86_64")
      ELSE ()
        SET (PKG_TARGET_ARCH "i386")
      ENDIF ()
    ENDIF ()
    set(PKG_TARGET "${PKG_TARGET}-${PKG_TARGET_ARCH}")
endif ()

if (NOT PKG_TARGET_ARCH)
    message(FATAL_ERROR "PluginSetup: no PKG_TARGET_ARCH defined")
endif ()

string(STRIP "${PKG_TARGET}" PKG_TARGET)
string(TOLOWER "${PKG_TARGET}" PKG_TARGET)
string(STRIP "${PKG_TARGET_VERSION}" PKG_TARGET_VERSION)
string(TOLOWER "${PKG_TARGET_VERSION}" PKG_TARGET_VERSION)
set(PKG_TARGET_NVR "${PKG_TARGET}-${PKG_TARGET_VERSION}")

set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}-${OCPN_MIN_VERSION}-${PKG_TARGET_NVR}")

set(PKG_UPLOAD_NAME "${VERBOSE_NAME}-${PKG_TARGET_NVR}")
set(PKG_TAR_FILE    "${CPACK_PACKAGE_FILE_NAME}.tar.gz")
set(PKG_XML_FILE    "${CPACK_PACKAGE_FILE_NAME}.xml")

message(STATUS "PluginSetup: PKG_TARGET: ${PKG_TARGET}")
message(STATUS "PluginSetup: PKG_TARGET_ARCH: ${PKG_TARGET_ARCH}")
message(STATUS "PluginSetup: PKG_TARGET_VERSION: ${PKG_TARGET_VERSION}")
message(STATUS "PluginSetup: PKG_TARGET_NVR: ${PKG_TARGET_NVR}")
message(STATUS "PluginSetup: PKG_UPLOAD_NAME: ${PKG_UPLOAD_NAME}")
message(STATUS "PluginSetup: PKG_TAR_FILE: ${PKG_TAR_FILE}")
message(STATUS "PluginSetup: PKG_XML_FILE: ${PKG_XML_FILE}")
message(STATUS "PluginSetup: CPACK_PACKAGE_FILE_NAME: ${CPACK_PACKAGE_FILE_NAME}")

