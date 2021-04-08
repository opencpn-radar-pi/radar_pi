# ~~~
# Author:      Pavel Kalian / Sean D'Epagnier
# Copyright:
# License:     GPLv3+
#
# Purpose:     Generate and install translations
# ~~~

find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
if (GETTEXT_XGETTEXT_EXECUTABLE)
  if (NOT TARGET pot-update)
    add_custom_target(pot-update COMMENT "Created pot file")
  endif ()
  add_custom_command(
    TARGET pot-update
    COMMAND
      ${GETTEXT_XGETTEXT_EXECUTABLE} --force-po --package-name=${PACKAGE_NAME}
      --package-version="${PROJECT_VERSION}" --output=po/${PACKAGE_NAME}.pot
      --keyword=_ --width=80
      --files-from=${CMAKE_BINARY_DIR}/POTFILES.in
    DEPENDS po/POTFILES.in po/${PACKAGE_NAME}.pot
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "pot-update: Generated pot file po/${PACKAGE_NAME}.pot."
  )
endif (GETTEXT_XGETTEXT_EXECUTABLE)

macro (gettext_update_po _potFile)
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
      COMMENT "po-update [${_poBasename}]: Updated po file."
    )
    set(_poFiles ${_poFiles} ${_absFile}.dummy)
  endforeach (_currentPoFile)
  if (NOT TARGET po-update)
    add_custom_target(
      po-update
      COMMENT "[${PACKAGE_NAME}]-po-update: Done."
      DEPENDS ${_poFiles}
    )
  endif ()
endmacro (gettext_update_po)

if (GETTEXT_MSGMERGE_EXECUTABLE)
  file(GLOB PACKAGE_PO_FILES po/*.po)
  gettext_update_po(po/${PACKAGE_NAME}.pot ${PACKAGE_PO_FILES})
endif (GETTEXT_MSGMERGE_EXECUTABLE)

set(_gmoFiles)
macro (gettext_build_mo)
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
      COMMENT "i18n [${_poBasename}]: Created mo file."
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
endmacro (gettext_build_mo)

if (GETTEXT_MSGFMT_EXECUTABLE)
  file(GLOB PACKAGE_PO_FILES po/*.po)
  gettext_build_mo(${PACKAGE_PO_FILES})
  if (NOT TARGET i18n)
    add_custom_target(
      i18n
      COMMENT "${PACKAGE_NAME}-i18n: Done."
      DEPENDS ${_gmoFiles}
    )
  endif ()
  add_dependencies(${PACKAGE_NAME} i18n)
endif (GETTEXT_MSGFMT_EXECUTABLE)
