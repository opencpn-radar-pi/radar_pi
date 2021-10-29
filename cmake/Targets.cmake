#
# Add the primary build targets pkg, flatpak and tarball together
# with helper targets.

#

if (TARGET tarball-build)
  return()
endif ()

include(Metadata)

if (UNIX AND NOT APPLE AND NOT QT_ANDROID)
  set(_LINUX ON)
else ()
  set(_LINUX OFF)
endif ()

if (WIN32)
  if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 16)
    message(WARNING "windows requires cmake version 3.16 or higher")
  endif ()
endif ()

# Set up _build_cmd
set(_build_cmd
  cmake --build ${CMAKE_BINARY_DIR} --parallel ${OCPN_NPROC} --config $<CONFIG>
)

# Set up _build_target_cmd and _install_cmd
if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 16)
  set(_build_target_cmd make)
  set(_install_cmd make install)
else ()
  set(_build_target_cmd
      cmake --build ${CMAKE_BINARY_DIR} --parallel ${OCPN_NPROC}
      --config $<CONFIG> --target
  )
  set(_install_cmd cmake --install ${CMAKE_BINARY_DIR} --config $<CONFIG>)
endif ()

# Command to remove directory
if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 17)
  set(_rmdir_cmd "remove_directory")
else ()
  set(_rmdir_cmd "rm -rf" )
endif ()


# Cmake batch file to compute and patch metadata checksum
#
set(_cs_script "
  execute_process(
    COMMAND  cmake -E sha256sum ${CMAKE_BINARY_DIR}/${pkg_tarname}.tar.gz
    OUTPUT_FILE ${CMAKE_BINARY_DIR}/${pkg_tarname}.sha256
  )
  file(READ ${CMAKE_BINARY_DIR}/${pkg_tarname}.sha256 _SHA256)
  string(REGEX MATCH \"^[^ ]*\" checksum \${_SHA256} )
  configure_file(
    ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml.in
    ${CMAKE_BINARY_DIR}/${pkg_xmlname}.xml
    @ONLY
  )
")
file(WRITE "${CMAKE_BINARY_DIR}/checksum.cmake" ${_cs_script})

function (create_finish_script)
  set(_finish_script "
    execute_process(
      COMMAND cmake -E ${_rmdir_cmd} app/${pkg_displayname}
    )
     execute_process(
      COMMAND cmake -E rename app/files app/${pkg_displayname}
    )
    execute_process(
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/app
      COMMAND
        cmake -E
        tar -czf ../${pkg_tarname}.tar.gz --format=gnutar ${pkg_displayname}
    )
    message(STATUS \"Creating tarball ${pkg_tarname}.tar.gz\")

    execute_process(COMMAND cmake -P ${CMAKE_BINARY_DIR}/checksum.cmake)
    message(STATUS \"Computing checksum in ${pkg_xmlname}.xml\")
  ")
  file(WRITE "${CMAKE_BINARY_DIR}/finish_tarball.cmake" "${_finish_script}")
endfunction ()

function (android_aarch64_target)
  add_custom_target(android-aarch64-conf)
  add_custom_command(
    TARGET android-aarch64-conf
    COMMAND cmake
      -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/app/files
      -DBUILD_TYPE:STRING=tarball
      -DOCPN_TARGET_TUPLE:STRING='android-arm64\;16\;arm64'
       ..
  )
  add_custom_target(android-aarch64-build)
  add_custom_command(TARGET android-aarch64-build COMMAND ${_build_cmd})

  add_custom_target(android-aarch64-install)
  add_custom_command(TARGET android-aarch64-install COMMAND ${_install_cmd})

  create_finish_script()
  add_custom_target(android-aarch64-finish)
  add_custom_command(
    TARGET android-aarch64-finish      # Compute checksum
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/finish_tarball.cmake
    VERBATIM
  )

  add_custom_target(android-aarch64)
  add_dependencies(android-aarch64 android-aarch64-finish)
  add_dependencies(android-aarch64-finish android-aarch64-install)
  add_dependencies(android-aarch64-install android-aarch64-build)
  add_dependencies(android-aarch64-build android-aarch64-conf)
endfunction ()

function (android_armhf_target)
  add_custom_target(android-armhf-conf)
  add_custom_command(
    TARGET android-armhf-conf
    COMMAND cmake
      -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/app/files
      -DBUILD_TYPE:STRING=tarball
      -DOCPN_TARGET_TUPLE:STRING='android-armhf\;16\;armhf'
       ..
  )
  add_custom_target(android-armhf-build)
  add_custom_command(TARGET android-armhf-build COMMAND ${_build_cmd})

  add_custom_target(android-armhf-install)
  add_custom_command(TARGET android-armhf-install COMMAND ${_install_cmd})

  create_finish_script()
  add_custom_target(android-armhf-finish)
  add_custom_command(
    TARGET android-armhf-finish      # Compute checksum
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/finish_tarball.cmake
    VERBATIM
  )

  add_custom_target(android-armhf)
  add_dependencies(android-armhf android-armhf-finish)
  add_dependencies(android-armhf-finish android-armhf-install)
  add_dependencies(android-armhf-install android-armhf-build)
  add_dependencies(android-armhf-build android-armhf-conf)
endfunction ()

function (tarball_target)

  # tarball target setup
  #
  add_custom_target(tarball-conf)
  add_custom_command(
    TARGET tarball-conf
    COMMAND cmake -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/app/files
            -DBUILD_TYPE:STRING=tarball ${CMAKE_BINARY_DIR}
  )

  add_custom_target(tarball-build)
  add_custom_command(TARGET tarball-build COMMAND ${_build_cmd})

  add_custom_target(tarball-install)
  add_custom_command(TARGET tarball-install COMMAND ${_install_cmd})

  create_finish_script()
  add_custom_target(tarball-finish)
  add_custom_command(
    TARGET tarball-finish      # Compute checksum
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/finish_tarball.cmake
    VERBATIM
  )
  add_dependencies(tarball-build tarball-conf)
  add_dependencies(tarball-install tarball-build)
  add_dependencies(tarball-finish tarball-install)

  add_custom_target(tarball)
  add_dependencies(tarball tarball-finish)
endfunction ()

function (flatpak_target manifest)

  add_custom_target(flatpak-conf)
  add_custom_command(
    TARGET flatpak-conf
    COMMAND
      cmake -DBUILD_TYPE:STRING=flatpak -Uplugin_target ${CMAKE_BINARY_DIR}
  )
  set(_fp_script "
    execute_process(
      COMMAND
        flatpak-builder --force-clean ${CMAKE_CURRENT_BINARY_DIR}/app
          ${manifest}
    )
    execute_process(
      COMMAND bash -c \"sed -e '/@checksum@/d' \
          < ${pkg_xmlname}.xml.in > app/files/metadata.xml\"
    )
    if (${CMAKE_BUILD_TYPE} MATCHES Release|MinSizeRel)
      message(STATUS \"Stripping app/files/lib/opencpn/lib${PACKAGE_NAME}.so\")
      execute_process(
        COMMAND strip app/files/lib/opencpn/lib${PACKAGE_NAME}.so
      )
    endif ()
    execute_process(
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/app
      COMMAND mv -fT files ${pkg_displayname}
    )
    execute_process(
      WORKING_DIRECTORY  ${CMAKE_BINARY_DIR}/app
      COMMAND
        cmake -E
        tar -czf ../${pkg_tarname}.tar.gz --format=gnutar ${pkg_displayname}
    )
    message(STATUS \"Building ${pkg_tarname}.tar.gz\")
    execute_process(
      COMMAND cmake -P ${CMAKE_BINARY_DIR}/checksum.cmake
    )
    message(STATUS \"Computing checksum in ${pkg_xmlname}.xml\")
  ")
  file(WRITE "${CMAKE_BINARY_DIR}/build_flatpak.cmake" ${_fp_script})
  add_custom_target(flatpak)
  add_custom_command(
    TARGET flatpak      # Compute checksum
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/build_flatpak.cmake
    VERBATIM
  )
  add_dependencies(flatpak flatpak-conf)
endfunction ()

function (help_target)

  # Help message for plain 'make' without target
  #
  add_custom_target(make-warning)
  add_custom_command(
    TARGET make-warning
    COMMAND cmake -E echo
      "ERROR: plain make is not supported. Supported targets:"
    COMMAND cmake -E echo
      "   - tarball: Plugin installer tarball for regular builds."
    COMMAND cmake -E echo
      "   - flatpak: Plugin installer tarball for flatpak builds."
    COMMAND cmake -E echo ""
    COMMAND dont-use-plain-make   # will fail
  )

  if ("${BUILD_TYPE}" STREQUAL "" )
    if ("${ARM_ARCH}" STREQUAL "aarch64")
      add_dependencies(${PACKAGE_NAME} android-aarch64)
    elseif ("${ARM_ARCH}" STREQUAL "armhf")
      add_dependencies(${PACKAGE_NAME} android-armhf)
    else ()
      add_dependencies(${PACKAGE_NAME} tarball)
    endif ()
  endif ()
endfunction ()

function (create_targets manifest)
  # Add the primary build targets pkg, flatpak and tarball together
  # with helper targets. Parameters:
  # - manifest: Flatpak build manifest

  if (BUILD_TYPE STREQUAL "pkg")
    message(FATAL_ERROR "Legacy package generation is not supported.")
  endif ()
  tarball_target()
  flatpak_target(${manifest})
  android_aarch64_target()
  android_armhf_target()
  help_target()
endfunction ()
