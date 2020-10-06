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
  set(_build_cmd
    cmake --build ${CMAKE_BINARY_DIR} ${_parallel_cmake_opt}
    --config $<CONFIG>
  )
endif ()

# Set up _build_target_cmd and _install_cmd
if (${CMAKE_MAJOR_VERSION} LESS 3 OR ${CMAKE_MINOR_VERSION} LESS 16)
  set(_build_target_cmd make)
  set(_install_cmd make install)
else ()
  set(_build_target_cmd
      cmake --build ${CMAKE_BINARY_DIR} --config $<CONFIG> --target
  )
  set(_install_cmd cmake --install ${CMAKE_BINARY_DIR} --config $<CONFIG>)
endif ()

if (WIN32 AND NOT MINGW)
  set(MVDIR rename)
else ()
  set(MVDIR mv)
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
  add_custom_command(
    TARGET tarball-build
    COMMAND ${_build_cmd}
  )
  add_custom_target(tarball-install)
  add_custom_command(
    TARGET tarball-install
    COMMAND ${_install_cmd}
  )
  add_custom_target(tarball-pkg)
  add_custom_target(
    TARGET tarball-pkg            # Move metadata in place.
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND cmake -E copy ${pkg_displayname}.xml app/files/metadata.xml
  )
  add_custom_target(tarball-finish)
  add_custom_command(
    TARGET tarball-finish     # Change top-level directory name
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/app
    COMMAND ${MVDIR} files ${pkg_displayname}
  )
  add_custom_target(tarball-tar)
  add_custom_command(
    TARGET tarball-tar
    WORKING_DIRECTORY  ${CMAKE_BINARY_DIR}/app
    COMMAND
      cmake -E
      tar -czf ../${pkg_tarname}.tar.gz --format=gnutar ${pkg_displayname}
    VERBATIM
    COMMENT "Building ${pkg_tarname}.tar.gz"
  )
  add_dependencies(tarball-build tarball-conf)
  add_dependencies(tarball-install tarball-build)
  add_dependencies(tarball-finish tarball-install)
  add_dependencies(tarball-pkg tarball-finish)
  add_dependencies(tarball-tar tarball-pkg)

  add_custom_target(tarball)
  add_dependencies(tarball tarball-tar)
endfunction ()

function (flatpak_target manifest)

  # flatpak target setup
  add_custom_target(flatpak-conf)
  add_custom_command(
    TARGET flatpak-conf
    COMMAND cmake -DBUILD_TYPE:STRING=flatpak -UPKG_TARGET ${CMAKE_BINARY_DIR}
  )
  add_custom_target(
    flatpak-build          # Build the package using flatpak-builder
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND flatpak-builder --force-clean ${CMAKE_CURRENT_BINARY_DIR}/app
            ${manifest}
  )
  add_custom_target(
    flatpak-pkg            # Move metadata in place.
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND cmake -E copy ${pkg_displayname}.xml app/files/metadata.xml
  )
  add_custom_target(
    flatpak-pkg-finish            # Change name of top directory
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/app
    COMMAND ${MVDIR} files ${pkg_displayname}
  )
  add_custom_target(flatpak-tar)
  add_custom_command(
    TARGET flatpak-tar
    WORKING_DIRECTORY  ${CMAKE_BINARY_DIR}/app
    COMMAND cmake -E
       tar -czf ../${pkg_tarname}.tar.gz --format=gnutar ${pkg_displayname}
    VERBATIM
    COMMENT "building ${pkg_tarname}.tar.gz"
  )
  add_dependencies(flatpak-build flatpak-conf)
  add_dependencies(flatpak-pkg flatpak-build)
  add_dependencies(flatpak-pkg-finish flatpak-pkg)
  add_dependencies(flatpak-tar flatpak-pkg-finish)

  add_custom_target(flatpak)
  add_dependencies(flatpak flatpak-tar)
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

