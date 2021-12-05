# ~~~
# Summary:      Generate and install translations
# Author:       Pavel Kalian / Sean D'Epagnier
# Copyright (c) 2014 Pavel Kallian
# License:      GPLv3+
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)

string(REPLACE "_pi" "" I18N_NAME ${PACKAGE_NAME})

if (GETTEXT_XGETTEXT_EXECUTABLE)
  add_custom_command(
    OUTPUT po/${PACKAGE_NAME}.pot.dummy
    COMMAND
      ${GETTEXT_XGETTEXT_EXECUTABLE} --force-po --package-name=${PACKAGE_NAME}
      --package-version="${PROJECT_VERSION}" --output=po/${PACKAGE_NAME}.pot
      --keyword=_ --width=80
      --files-from=${CMAKE_CURRENT_SOURCE_DIR}/po/POTFILES.in
    DEPENDS po/POTFILES.in po/${PACKAGE_NAME}.pot
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "${I18N_NAME}-pot-update [${PACKAGE_NAME}]: Generated pot file."
  )
  add_custom_target(
    ${I18N_NAME}-pot-update
    COMMENT "[${PACKAGE_NAME}]-pot-update: Done."
    DEPENDS po/${PACKAGE_NAME}.pot.dummy
  )
endif (GETTEXT_XGETTEXT_EXECUTABLE)

macro (GETTEXT_UPDATE_PO _potFile)
  set(_poFiles ${_potFile})
  get_filename_component(_absPotFile ${_potFile} ABSOLUTE)

  foreach (_currentPoFile ${ARGN})
    get_filename_component(_absFile ${_currentPoFile} ABSOLUTE)
    get_filename_component(_poBasename ${_absFile} NAME_WE)

    add_custom_command(
      OUTPUT ${_absFile}.dummy
      COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --width=80 --strict --quiet
              --update --backup=none --no-location -s ${_absFile} ${_absPotFile}
      DEPENDS ${_absPotFile} ${_absFile}
      COMMENT "${I18N_NAME}-po-update [${_poBasename}]: Updated po file."
    )
    set(_poFiles ${_poFiles} ${_absFile}.dummy)
  endforeach (_currentPoFile)

  add_custom_target(
    ${I18N_NAME}-po-update
    COMMENT "[${PACKAGE_NAME}]-po-update: Done."
    DEPENDS ${_poFiles}
  )
endmacro (GETTEXT_UPDATE_PO)

if (GETTEXT_MSGMERGE_EXECUTABLE)
  file(GLOB PACKAGE_PO_FILES po/*.po)
  gettext_update_po(po/${PACKAGE_NAME}.pot ${PACKAGE_PO_FILES})
endif (GETTEXT_MSGMERGE_EXECUTABLE)

set(_gmoFiles)
macro (GETTEXT_BUILD_MO)
  foreach (_poFile ${ARGN})
    get_filename_component(_absFile ${_poFile} ABSOLUTE)
    get_filename_component(_poBasename ${_absFile} NAME_WE)
    set(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_poBasename}.mo)

    add_custom_command(
      OUTPUT ${_gmoFile}
      COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --check -o ${_gmoFile} ${_absFile}
      COMMAND ${CMAKE_COMMAND} -E copy ${_gmoFile}
              "Resources/${_poBasename}.lproj/opencpn-${PACKAGE_NAME}.mo"
      DEPENDS ${_absFile}
      COMMENT "${I18N_NAME}-i18n [${_poBasename}]: Created mo file."
    )
    if (APPLE)
      install(
        FILES ${_gmoFile}
        DESTINATION OpenCPN.app/Contents/Resources/${_poBasename}.lproj
        RENAME opencpn-${PACKAGE_NAME}.mo
      )
    else (APPLE)
      install(
        FILES ${_gmoFile}
        DESTINATION share/locale/${_poBasename}/LC_MESSAGES
        RENAME opencpn-${PACKAGE_NAME}.mo
      )
    endif (APPLE)

    set(_gmoFiles ${_gmoFiles} ${_gmoFile})
  endforeach (_poFile)
endmacro (GETTEXT_BUILD_MO)

if (GETTEXT_MSGFMT_EXECUTABLE)
  file(GLOB PACKAGE_PO_FILES po/*.po)
  gettext_build_mo(${PACKAGE_PO_FILES})
  add_custom_target(
    ${I18N_NAME}-i18n
    COMMENT "${PACKAGE_NAME}-i18n: Done."
    DEPENDS ${_gmoFiles}
  )
  add_dependencies(${PACKAGE_NAME} ${I18N_NAME}-i18n)
  add_dependencies( tarball ${I18N_NAME}-i18n)
endif (GETTEXT_MSGFMT_EXECUTABLE)
