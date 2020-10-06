#~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
#
# Set up CPack package generation.
#~~~

include(GetArch)
include(Metadata)

set(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "opencpn.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_NAME} ${PACKAGE_VERSION})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_CURRENT_BINARY_DIR};${PACKAGE_NAME};ALL;/"
)
set(CPACK_PACKAGE_EXECUTABLES OpenCPN ${PACKAGE_NAME})
set(CPACK_PACKAGE_FILE_NAME "${pkg_tarname}")

if (WIN32)
  set(CPACK_GENERATOR "NSIS")

  # override install directory to put package files in the opencpn directory
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenCPN")

  # CPACK_NSIS_DIR ?? CPACK_BUILDWIN_DIR ?? CPACK_PACKAGE_ICON ??

  set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}-${OCPN_MIN_VERSION}")

  # These lines set the name of the Windows Start Menu shortcut and the icon
  # that goes with it SET(CPACK_NSIS_INSTALLED_ICON_NAME "${PACKAGE_NAME}")
  set(CPACK_NSIS_DISPLAY_NAME "OpenCPN ${PACKAGE_NAME}")

  # SET(CPACK_PACKAGE_FILE_NAME
  # "${PACKAGE_NAME}_${VERSION_MAJOR}.${VERSION_MINOR}_setup" )

  set(CPACK_NSIS_DIR "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode") # Gunther
  set(CPACK_BUILDWIN_DIR "${PROJECT_SOURCE_DIR}/buildwin") # Gunther

else (WIN32)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PACKAGE_NAME})
endif (WIN32)

set(CPACK_STRIP_FILES "${PACKAGE_NAME}")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README")
  # MESSAGE(STATUS "Using generic cpack package description file.")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
endif ()

# The following components are regex's to match anywhere (unless anchored) in
# absolute path + filename to find files or directories to be excluded from
# source tarball.
set(CPACK_SOURCE_IGNORE_FILES
    "^${CMAKE_CURRENT_SOURCE_DIR}/.git/*" "^${CMAKE_CURRENT_SOURCE_DIR}/build*"
    "^${CPACK_PACKAGE_INSTALL_DIRECTORY}/*"
)

if (APPLE)
  set(CPACK_GENERATOR ZIP)   # make sure we don't overwrite 'make tarball''

elseif (UNIX)

  set(PACKAGE_DEPS opencpn)
  # autogenerate additional dependency information
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  set(PACKAGE_RELEASE ${PKG_RELEASE})

  set(CPACK_GENERATOR "DEB")

  set(CPACK_DEBIAN_PACKAGE_DEPENDS ${PACKAGE_DEPS})
  set(CPACK_DEBIAN_PACKAGE_RECOMMENDS ${PACKAGE_RECS})
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCH})
  set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
  set(CPACK_DEBIAN_PACKAGE_SECTION "misc")
  set(CPACK_DEBIAN_COMPRESSION_TYPE "xz") # requires my patches to cmake

  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_NAME} PlugIn for OpenCPN")
  set(CPACK_PACKAGE_DESCRIPTION "${PACKAGE_NAME} PlugIn for OpenCPN")
  # SET(CPACK_SET_DESTDIR ON)
  set(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

endif ()

if (TWIN32 AND NOT UNIX)
  # configure_file(${PROJECT_SOURCE_DIR}/src/opencpn.rc.in
  # ${PROJECT_SOURCE_DIR}/src/opencpn.rc)
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_GERMAN.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_GERMAN.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_FRENCH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_FRENCH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_CZECH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_CZECH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_DANISH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_DANISH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_SPANISH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_SPANISH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_ITALIAN.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_ITALIAN.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_DUTCH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_DUTCH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_POLISH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_POLISH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_PORTUGUESEBR.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_PORTUGUESEBR.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_PORTUGUESE.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_PORTUGUESE.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_RUSSIAN.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_RUSSIAN.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_SWEDISH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_SWEDISH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_FINNISH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_FINNISH.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_NORWEGIAN.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_NORWEGIAN.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_CHINESETW.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_CHINESETW.nsh"
    @ONLY
  )
  configure_file(
    "${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_TURKISH.nsh.in"
    "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_TURKISH.nsh"
    @ONLY
  )
endif (TWIN32 AND NOT UNIX)

include(CPack)

if (APPLE)

       #  Copy a bunch of files so the Packages installer builder can find them
 #  relative to ${CMAKE_CURRENT_BINARY_DIR}
 #  This avoids absolute paths in the chartdldr_pi.pkgproj file

configure_file(${PROJECT_SOURCE_DIR}/cmake/gpl.txt
            ${CMAKE_CURRENT_BINARY_DIR}/license.txt COPYONLY)

configure_file(${PROJECT_SOURCE_DIR}/buildosx/InstallOSX/pkg_background.jpg
            ${CMAKE_CURRENT_BINARY_DIR}/pkg_background.jpg COPYONLY)

 # Patch the pkgproj.in file to make the output package name conform to Xxx-Plugin_x.x.pkg format
 #  Key is:
 #  <key>NAME</key>
 # <string>${VERBOSE_NAME}-Plugin_${VERSION_MAJOR}.${VERSION_MINOR}</string>

 configure_file(${PROJECT_SOURCE_DIR}/buildosx/InstallOSX/${PACKAGE_NAME}.pkgproj.in
            ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}.pkgproj)

 ADD_CUSTOM_COMMAND(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}-Plugin.pkg
   COMMAND /usr/local/bin/packagesbuild -F ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}.pkgproj
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   DEPENDS ${PACKAGE_NAME}
   COMMENT "create-pkg [${PACKAGE_NAME}]: Generating pkg file."
)

 ADD_CUSTOM_TARGET(create-pkg COMMENT "create-pkg: Done."
 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}-Plugin.pkg )

 SET(CPACK_GENERATOR "TGZ")

endif (APPLE)

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
