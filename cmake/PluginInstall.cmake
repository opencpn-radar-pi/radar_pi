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
    RUNTIME
    LIBRARY DESTINATION OpenCPN.app/Contents/SharedSupport/plugins
  )
  install(
    TARGETS ${PACKAGE_NAME}
    RUNTIME
    LIBRARY DESTINATION OpenCPN.app/Contents/PlugIns
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
    set(INSTALL_DIRECTORY "plugins/${PACKAGE_NAME}")
  else ()
    install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
    set(INSTALL_DIRECTORY "plugins\\\\${PACKAGE_NAME}")
  endif ()

  if (EXISTS ${PROJECT_SOURCE_DIR}/data)
    install(DIRECTORY data DESTINATION "${INSTALL_DIRECTORY}")
  endif (EXISTS ${PROJECT_SOURCE_DIR}/data)

elseif (UNIX AND NOT APPLE)
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
  install(FILES ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml
          DESTINATION "${CMAKE_BINARY_DIR}/app/files"
          RENAME metadata.xml
  )
endif()
