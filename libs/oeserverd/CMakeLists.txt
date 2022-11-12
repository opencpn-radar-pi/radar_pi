#
# Install the pre-compiled oexserverd DRM module + helper libraries.
#
macro (install_libs libs)
  if (APPLE)
    #set(_oe_libdir "OpenCPN.app/Contents/SharedSupport/plugins/${PACKAGE_NAME}")
    set(_oe_libdir "OpenCPN.app/Contents/PlugIns/")
  elseif (WIN32)
    set(_oe_libdir "plugins")
  elseif (UNIX)
    set(_oe_libdir "lib/opencpn/")
  endif ()
  foreach(lib ${ARGV})
    install(
      FILES ${lib}
      PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE
                  WORLD_READ WORLD_EXECUTE
      DESTINATION ${CMAKE_INSTALL_PREFIX}/${_oe_libdir}
    )
  endforeach ()
endmacro () 

if (QT_ANDROID)
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/oeserverda_android/oexserverd
    ${CMAKE_INSTALL_PREFIX}/bin/oexserverd COPYONLY
  )
elseif (APPLE)
  install_libs("osx/oexserverd" "osx/libsglmac-2.38.0.0.dylib.x86_64")
elseif (UNIX AND ARCH  STREQUAL "x86_64")
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linux64/oexserverd
    ${CMAKE_INSTALL_PREFIX}/bin/oexserverd COPYONLY
  )
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linux64/libsgllnx64-2.29.02.so
    ${CMAKE_INSTALL_PREFIX}/bin/libsgllnx64-2.29.02.so COPYONLY
  )
elseif (UNIX AND ARCH STREQUAL "armhf")
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linuxarm/oexserverd
    ${CMAKE_INSTALL_PREFIX}/bin/oexserverd COPYONLY
  )
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linuxarm/libsglarmhf32-2.30.0.0.so
    ${CMAKE_INSTALL_PREFIX}/bin/libsglarmhf32-2.30.0.0.so COPYONLY
  )
elseif (UNIX AND ARCH STREQUAL "arm64")
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linuxarm64/oexserverd
    ${CMAKE_INSTALL_PREFIX}/bin/oexserverd COPYONLY
  )
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linuxarm64/libsglarm64-2.31.0.0.so
    ${CMAKE_INSTALL_PREFIX}/bin/libsglarm64-2.31.0.0.so COPYONLY
  )
elseif (UNIX)
  # 32-bit amd64: is this really useful?
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linux/oeserverd
    ${CMAKE_INSTALL_PREFIX}/bin/oeserverd COPYONLY
  )
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linux/libsgllnx-2.29.0.1.so
    ${CMAKE_INSTALL_PREFIX}/bin/libsgllnx-2.29.0.1.so COPYONLY
  )
elseif (WIN32)
  file(GLOB_RECURSE vcdll_files "win/*.dll")
  message(STATUS "GLOB vcdll_files: ${vcdll_files}")
  install_libs(${vcdll_files} ${CMAKE_CURRENT_LIST_DIR}/win/oexserverd.exe)
endif ()
