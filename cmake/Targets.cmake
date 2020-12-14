#
# Add the primary build targets pkg, flatpak and tarball together with helper
# targets.
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

# Set up _parallel_cmake_opt
if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 12)
  set(_parallel_cmake_opt "")
else ()
  set(_parallel_cmake_opt "--parallel")
endif ()

# Set up _build_cmd
if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 10)
  set(_build_cmd make -j2)
else ()
  set(_build_cmd cmake --build ${CMAKE_BINARY_DIR} ${_parallel_cmake_opt})
endif ()

# Set up _build_target_cmd and _install_cmd
if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 16)
  set(_build_target_cmd make)
  set(_install_cmd make install)
else ()
  set(_build_target_cmd cmake --build ${CMAKE_BINARY_DIR} --config $<CONFIG>
                        --target
  )
  set(_install_cmd cmake --install ${CMAKE_BINARY_DIR} --config $<CONFIG>)
endif ()

if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 12)
  set(_mvdir mv)
else ()
  set(_mvdir cmake -E rename)
endif ()

# Command to compute sha256 checksum
if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 10)
  set(_cs_command "${pkg_python} ${PROJECT_SOURCE_DIR}/ci/checksum.py")
else ()
  set(_cs_command "cmake -E sha256sum")
endif ()

# Cmake batch file to compute and patch metadata checksum
#
set(_cs_script
    "
  execute_process(
    COMMAND ${_cs_command}  ${CMAKE_BINARY_DIR}/${pkg_tarname}.tar.gz
    OUTPUT_FILE ${CMAKE_BINARY_DIR}/${pkg_tarname}.sha256
  )
  file(READ ${CMAKE_BINARY_DIR}/${pkg_tarname}.sha256 _SHA256)
  string(REGEX MATCH \"^[^ ]*\" checksum \${_SHA256} )
  configure_file(
    ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml.in
    ${CMAKE_BINARY_DIR}/${pkg_displayname}.xml
    @ONLY
  )
"
)
file(WRITE "${CMAKE_BINARY_DIR}/checksum.cmake" ${_cs_script})

function (tarball_target)

  add_custom_target(tarball-conf)
  add_custom_command(
    TARGET tarball-conf
    COMMAND cmake -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/app/files
            -DBUILD_TYPE:STRING=tarball ${CMAKE_BINARY_DIR}
  )
  set(_tarball_script
      "
    execute_process(COMMAND ${_build_cmd})
    execute_process(COMMAND ${_install_cmd})
    execute_process(
      WORKING_DIRECTORY  ${CMAKE_BINARY_DIR}/app
      COMMAND ${_mvdir} files ${pkg_displayname}
    )
    execute_process(
      WORKING_DIRECTORY  ${CMAKE_BINARY_DIR}/app
      COMMAND
        cmake -E
        tar -czf ../${pkg_tarname}.tar.gz --format=gnutar ${pkg_displayname}
    )
    message(STATUS \"Creating tarball ${pkg_tarname}.tar.gz\")

    execute_process( COMMAND cmake -P ${CMAKE_BINARY_DIR}/checksum.cmake)
    message(STATUS \"Computing checksum in ${pkg_displayname}.xml\")
  "
  )
  file(WRITE "${CMAKE_BINARY_DIR}/build_tarball.cmake" "${_tarball_script}")
  add_custom_target(tarball)
  add_custom_command(
    TARGET tarball # Compute checksum
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/build_tarball.cmake
    VERBATIM
  )
  add_dependencies(tarball tarball-conf)
endfunction ()

function (flatpak_target manifest)

  add_custom_target(flatpak-conf)
  add_custom_command(
    TARGET flatpak-conf COMMAND cmake -DBUILD_TYPE:STRING=flatpak
                                -Uplugin_target ${CMAKE_BINARY_DIR}
  )
  add_custom_target(flatpak-build)
  set(_fp_script
      "
    execute_process(
      COMMAND
        flatpak-builder --force-clean ${CMAKE_CURRENT_BINARY_DIR}/app
          ${manifest}
    )
    execute_process(
      COMMAND bash -c \"sed -e '/@checksum@/d' \
          < ${pkg_displayname}.xml.in > app/files/metadata.xml\"
    )
    execute_process(
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/app
      COMMAND mv files ${pkg_displayname}
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
  "
  )
  file(WRITE "${CMAKE_BINARY_DIR}/build_flatpak.cmake" "${_fp_script}")
  add_custom_target(flatpak)
  add_custom_command(
    TARGET flatpak # Compute checksum
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
    TARGET pkg-conf COMMAND cmake -DBUILD_TYPE:STRING=pkg ${CMAKE_BINARY_DIR}
  )
  add_custom_target(pkg-build)
  add_custom_command(TARGET pkg-build COMMAND ${_build_cmd})

  add_custom_target(pkg-package)
  add_custom_command(
    TARGET pkg-package
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND cpack $<$<BOOL:$<CONFIG>>:"-C $<CONFIG>">
  )
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
    COMMAND dont-use-plain-make # will fail
  )

  if ("${BUILD_TYPE}" STREQUAL "")
    add_dependencies(${PACKAGE_NAME} make-warning)
  endif ()
endfunction ()

function (create_targets manifest)
  # Add the primary build targets pkg, flatpak and tarball together with helper
  # targets. Parameters: - manifest: Flatpak build manifest

  tarball_target()
  flatpak_target(${manifest})
  pkg_target()
  help_target()
endfunction ()
