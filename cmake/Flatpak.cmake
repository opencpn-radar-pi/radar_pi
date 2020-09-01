#~~~
# Add the targets flatpak-build and flatpak-pkg which creates the
# flatpak tarball. Parameters
#   - manifest: The flatpak .yaml manifest 
#   - tarname: Name of generated tarball
#~~~

function (FlatpakTargets manifest tarname)
  find_program(TAR NAMES gtar tar)
  if (NOT TAR)
    message(FATAL_ERROR "tar not found, required for OCPN_FLATPAK")
  endif ()
  execute_process(
    # Get the part after last dot in the manifest id: stanza
    COMMAND /bin/bash -c "sed -n /^id:/s/.*\\\\.//p <  ${manifest}"
    OUTPUT_VARIABLE plugin_name
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  add_custom_target(
    flatpak-build          # Build the package using flatpak-builder
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND flatpak-builder --force-clean ${CMAKE_CURRENT_BINARY_DIR}/app
            ${manifest} 
  )
  add_custom_target(
    flatpak-pkg            # Create tarball
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND
      ${TAR} -czf ${tarname}
      --transform 's|.*/files/|${plugin_name}-flatpak-${PACKAGE_VERSION}/|'
      ${CMAKE_CURRENT_BINARY_DIR}/app/files
  )
  add_custom_target(
    fp-warning
    COMMAND echo "-- WARNING: Use flatpak-build to build flatpak\n\n"
    COMMAND dont-use-plain-make-when-doing-flatpak   # will fail
  )
  add_dependencies(flatpak-pkg flatpak-build)
  add_dependencies(repack-tarball flatpak-pkg)

  add_dependencies(${PACKAGE_NAME} fp-warning)
endfunction ()
