# ~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
#
# Installation items and layout.
# ~~~

include(Metadata)

if (APPLE)
  install(
    TARGETS ${PACKAGE_NAME}
    RUNTIME LIBRARY DESTINATION OpenCPN.app/Contents/PlugIns
  )
  if (EXISTS ${PROJECT_SOURCE_DIR}/data)
    install(
      DIRECTORY data
      DESTINATION OpenCPN.app/Contents/SharedSupport/plugins/${PACKAGE_NAME}
    )
  endif ()

elseif (WIN32)
  message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
  if (CMAKE_CROSSCOMPILING)
    install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
  else ()
    install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
  endif ()
  if (EXISTS ${PROJECT_SOURCE_DIR}/data)
    install(DIRECTORY data DESTINATION "plugins/${PACKAGE_NAME}")
  endif ()

elseif (UNIX)
  install(
    TARGETS ${PACKAGE_NAME}
    RUNTIME LIBRARY DESTINATION lib/opencpn
  )
  if (EXISTS ${PROJECT_SOURCE_DIR}/data)
    install(DIRECTORY data DESTINATION share/opencpn/plugins/${PACKAGE_NAME})
  endif ()
endif ()

# Hardcoded, absolute destination for tarball generation
if (${BUILD_TYPE} STREQUAL "tarball" OR ${BUILD_TYPE} STREQUAL "flatpak")
  install(CODE "
    configure_file(
      ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml.in
      ${CMAKE_BINARY_DIR}/app/files/metadata.xml
      @ONLY
    )
  ")
endif()

# On macos, fix paths which points to the build environment, make sure they
# refers to runtime locations
if (${BUILD_TYPE} STREQUAL "tarball" AND APPLE)
  install(CODE
    "execute_process(
      COMMAND bash -c ${PROJECT_SOURCE_DIR}/cmake/fix-macos-libs.sh
    )"
  )
endif()

if (CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel")
  if (APPLE)
    set(_striplib OpenCPN.app/Contents/PlugIns/lib${PACKAGE_NAME}.dylib)
  elseif (MINGW)
    set(_striplib plugins/lib${PACKAGE_NAME}.dll)
  elseif (UNIX AND NOT CMAKE_CROSSCOMPILING AND NOT DEFINED ENV{FLATPAK_ID})
    # Plain, native linux
    set(_striplib lib/opencpn/lib${PACKAGE_NAME}.so)
  endif ()
  if (BUILD_TYPE STREQUAL "tarball" AND DEFINED _striplib)
    find_program(STRIP_UTIL NAMES strip REQUIRED)
    if (APPLE)
      set(STRIP_UTIL "${STRIP_UTIL} -x")
    endif ()
    install(CODE "message(STATUS \"Stripping ${_striplib}\")")
    install(CODE "
      execute_process(
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${STRIP_UTIL} app/files/${_striplib}
      )
    ")
  endif ()
endif ()
