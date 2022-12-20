# ~~~
# Summary:     Add primary build targets
# License:     GPLv3+
# Copyright (c) 2020-2021 Alec Leamas
#
# Add the primary build targets android, flatpak and tarball together
# with helper targets. Also sets up the default target.
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.




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
  if (CMAKE_VERSION VERSION_LESS 3.16)
    message(WARNING "windows requires cmake version 3.16 or higher")
  endif ()
endif ()

# Set up _build_cmd
set(_build_cmd
  cmake --build ${CMAKE_BINARY_DIR} --parallel ${OCPN_NPROC} --config $<CONFIG>
)

# Set up _build_target_cmd and _install_cmd
if (CMAKE_VERSION VERSION_LESS 3.16)
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
if (CMAKE_VERSION VERSION_LESS 3.17)
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

function (android_target)
  if ("${ARM_ARCH}" STREQUAL "aarch64")
    set(OCPN_TARGET_TUPLE "'android-arm64\;16\;arm64'")
  else ()
    set(OCPN_TARGET_TUPLE "'android-armhf\;16\;armhf'")
  endif ()
  add_custom_command(
    OUTPUT android-conf-stamp
    COMMAND cmake -E touch android-conf-stamp
    COMMAND cmake
      -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/app/files
      -DBUILD_TYPE:STRING=tarball
      -DOCPN_TARGET_TUPLE:STRING=${OCPN_TARGET_TUPLE}
      $ENV{CMAKE_BUILD_OPTS}
      ${CMAKE_BINARY_DIR}
  )
  add_custom_target(android-build DEPENDS android-conf-stamp)
  add_custom_command(
    TARGET android-build COMMAND ${_build_target_cmd} ${PKG_NAME}
  )
  add_custom_target(android-install)
  add_custom_command(TARGET android-install COMMAND ${_install_cmd})

  create_finish_script()
  add_custom_target(android-finish)
  add_custom_command(
    TARGET android-finish
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/finish_tarball.cmake
    VERBATIM
  )
  add_custom_target(android)
  add_dependencies(android android-finish)
  add_dependencies(android-finish android-install)
  add_dependencies(android-install android-build)
endfunction ()

function (tarball_target)

  # tarball target setup
  #
  add_custom_command(
    OUTPUT tarball-conf-stamp
    COMMAND cmake -E touch tarball-conf-stamp
    COMMAND cmake
      -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/app/files
      -DBUILD_TYPE:STRING=tarball
      $ENV{CMAKE_BUILD_OPTS}
      ${CMAKE_BINARY_DIR}
  )

  add_custom_target(tarball-build DEPENDS tarball-conf-stamp)
  add_custom_command(
    TARGET tarball-build COMMAND ${_build_target_cmd} ${PKG_NAME}
  )

  add_custom_target(tarball-install)
  add_custom_command(TARGET tarball-install COMMAND ${_install_cmd})

  create_finish_script()
  add_custom_target(tarball-finish)
  add_custom_command(
    TARGET tarball-finish      # Compute checksum
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/finish_tarball.cmake
    VERBATIM
  )
  add_dependencies(tarball-install tarball-build)
  add_dependencies(tarball-finish tarball-install)

  add_custom_target(tarball)
  add_dependencies(tarball tarball-finish)
endfunction ()

function (flatpak_target manifest)

  add_custom_target(flatpak-conf)
  add_custom_command(
    TARGET flatpak-conf
    COMMAND cmake
      -DBUILD_TYPE:STRING=flatpak
      -Uplugin_target
      $ENV{CMAKE_BUILD_OPTS}
      ${CMAKE_BINARY_DIR}
  )

  # Script used to copy out files from the flatpak sandbox
  file(WRITE ${CMAKE_BINARY_DIR}/copy_out [=[
    appdir=$(find /run/build -maxdepth 3 -iname $1)
    appdir=$(ls -t $appdir)       # Sort entries if there is more than one
    appdir=${appdir%% *}          # Pick first entry
    appdir=${appdir%%/lib*so}     # Drop filename, use remaining dir part
    cp -ar $appdir/app $2
  ]=])

  set(_fp_script "
    execute_process(
      COMMAND
        flatpak-builder --force-clean --keep-build-dirs
          ${CMAKE_BINARY_DIR}/app ${manifest}
    )
    # Copy the data out of the sandbox to installation directory
    execute_process(
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMAND
        flatpak-builder --run app ${manifest}
           bash copy_out lib${PACKAGE_NAME}.so ${CMAKE_BINARY_DIR}
    )
    if (NOT EXISTS app/files/lib/opencpn/lib${PACKAGE_NAME}.so)
      message(FATAL_ERROR \"Cannot find generated file lib${PACKAGE_NAME}.so\")
    endif ()
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
    TARGET flatpak
    COMMAND cmake -P ${CMAKE_BINARY_DIR}/build_flatpak.cmake
    VERBATIM
  )
  add_dependencies(flatpak flatpak-conf)
endfunction ()

function (create_targets manifest)
  # Add the primary build targets android, flatpak and tarball together
  # with support targets. Parameters:
  # - manifest: Flatpak build manifest

  if (BUILD_TYPE STREQUAL "pkg")
    message(FATAL_ERROR "Legacy package generation is not supported.")
  endif ()
  tarball_target()
  flatpak_target(${manifest})
  android_target()
  add_custom_target(default ALL)
  if ("${ARM_ARCH}" STREQUAL "")
    add_dependencies(default tarball)
  else ()
    add_dependencies(default android)
  endif ()
endfunction ()
