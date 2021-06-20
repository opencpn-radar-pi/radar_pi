#
# Add the primary build targets pkg, flatpak and tarball together
# with helper targets.

#

if (TARGET tarball-build)
  return()
endif ()

include(Metadata)

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
    ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml
    @ONLY
  )
")
file(WRITE "${CMAKE_BINARY_DIR}/checksum.cmake" ${_cs_script})

# Command to build legacy package
if (APPLE)
    set(_build_pkg_cmd ${_build_target_cmd} create-pkg)
else ()
    set(_build_pkg_cmd ${_build_target_cmd} package)
endif ()


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
    message(STATUS \"Computing checksum in ${pkg_displayname}.xml\")
  ")
  file(WRITE "${CMAKE_BINARY_DIR}/finish_tarball.cmake" "${_finish_script}")
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
          < ${pkg_displayname}.xml.in > app/files/metadata.xml\"
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
    message(STATUS \"Computing checksum in ${pkg_displayname}.xml\")
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

function (pkg_target)

  # pkg target setup
  #
  add_custom_target(pkg-conf)
  add_custom_command(
    TARGET pkg-conf
    COMMAND cmake -DBUILD_TYPE:STRING=pkg ${CMAKE_BINARY_DIR}
  )
  add_custom_target(pkg-build)
  add_custom_command(TARGET pkg-build COMMAND ${_build_cmd})

  add_custom_target(pkg-package)
  add_custom_command(TARGET pkg-package COMMAND ${_build_pkg_cmd})

  add_dependencies(pkg-build pkg-conf)
  add_dependencies(pkg-package pkg-build)

  add_custom_target(pkg)
  add_dependencies(pkg pkg-package)
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
    COMMAND cmake -E echo
      "   - pkg: Legacy installer package on Windows, Mac and Debian."
    COMMAND cmake -E echo ""
    COMMAND dont-use-plain-make   # will fail
  )

  if ("${BUILD_TYPE}" STREQUAL "" )
    add_dependencies(${PACKAGE_NAME} make-warning)
  endif ()
endfunction ()

function (create_targets manifest)
  # Add the primary build targets pkg, flatpak and tarball together
  # with helper targets. Parameters:
  # - manifest: Flatpak build manifest

  tarball_target()
  flatpak_target(${manifest})
  pkg_target()
  help_target()
endfunction ()
