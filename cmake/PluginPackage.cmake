#~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
#
# Set up CPack package generation.
#~~~

include(GetArch)
include(Metadata)

set(CPACK_PACKAGE_VENDOR "opencpn.org")
set(CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_CURRENT_BINARY_DIR};${PACKAGE_NAME};ALL;/"
)
set(CPACK_PACKAGE_FILE_NAME "${pkg_tarname}")

set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
if (EXISTS "${PROJECT_SOURCE_DIR}/README.md")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
  set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
endif ()

set(CPACK_STRIP_FILES "${PACKAGE_NAME}")

# The following components are regex's to match anywhere (unless anchored) in
# absolute path + filename to find files or directories to be excluded from
# source tarball.
set(CPACK_SOURCE_IGNORE_FILES
  "^${PROJECT_SOURCE_DIR}/.git/*" "^${PROJECT_SOURCE_DIR}/build*"
    "^${CPACK_PACKAGE_INSTALL_DIRECTORY}/*"
)

if (WIN32)
  set(CPACK_GENERATOR "NSIS")

  # override install directory to put package files in the opencpn directory
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenCPN")

  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}-${OCPN_MIN_VERSION}")

  # Menu label in Add/Remove program etc.
  set(CPACK_NSIS_DISPLAY_NAME "OpenCPN ${PACKAGE_NAME}")
  set(CPACK_NSIS_DIR "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode") # Gunther
  set(CPACK_BUILDWIN_DIR "${PROJECT_SOURCE_DIR}/buildwin") # Gunther

else ()
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PACKAGE_NAME})
endif ()

if (APPLE)
  set(CPACK_GENERATOR ZIP)   # make sure we don't overwrite 'make tarball''

elseif (UNIX)
  set(PACKAGE_DEPS opencpn)
  # autogenerate additional dependency information
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  set(CPACK_GENERATOR "DEB")

  set(CPACK_DEBIAN_PACKAGE_DEPENDS ${PACKAGE_DEPS})
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCH})
  set(CPACK_DEBIAN_PACKAGE_SECTION "misc")
  set(CPACK_DEBIAN_COMPRESSION_TYPE "xz") # requires my patches to cmake

endif ()

include(CPack)

if (WIN32)
  message(STATUS "FILE: ${CPACK_PACKAGE_FILE_NAME}")
  add_custom_command(
    OUTPUT ${CPACK_PACKAGE_FILE_NAME}
    COMMAND
      signtool sign /v /f \\cert\\OpenCPNSPC.pfx /d http://www.opencpn.org /t
      http://timestamp.verisign.com/scripts/timstamp.dll
      ${CPACK_PACKAGE_FILE_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${PACKAGE_NAME}
    COMMENT "Code-Signing: ${CPACK_PACKAGE_FILE_NAME}"
  )
  add_custom_target(
    codesign
    COMMENT "code signing: Done."
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}
  )
endif ()
