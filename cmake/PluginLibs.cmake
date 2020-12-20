#
# Find and link  general libraries to use, basically gettext and wxWidgets

find_package(Gettext REQUIRED)

set(wxWidgets_USE_DEBUG OFF)
set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_UNIVERSAL OFF)
set(wxWidgets_USE_STATIC OFF)
if (NOT QT_ANDROID)
  # QT_ANDROID is a cross-build, so the native FIND_PACKAGE(OpenGL) is
  # not useful.
  find_package(OpenGL)
  if (OPENGL_GLU_FOUND)
    set(GL_LIB gl)
    include_directories(${OPENGL_INCLUDE_DIR})
    message(STATUS "Found OpenGL...")
    message(STATUS "    Lib: " ${OPENGL_LIBRARIES})
    message(STATUS "    Include: " ${OPENGL_INCLUDE_DIR})
    add_definitions(-DocpnUSE_GL)
  else ()
    message(STATUS "OpenGL not found...")
  endif ()
  
  set(wxWidgets_USE_LIBS base core net xml html adv stc)
  set(BUILD_SHARED_LIBS TRUE)

  find_package(wxWidgets REQUIRED base core net xml html adv stc)

  if(MSYS)
  # this is just a hack. I think the bug is in FindwxWidgets.cmake
    string( REGEX REPLACE "/usr/local" "\\\\;C:/MinGW/msys/1.0/usr/local" wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} )
  endif()

  include(${wxWidgets_USE_FILE})	
	
endif ()


if (MINGW)
  target_link_libraries(${PACKAGE_NAME} ${OPENGL_LIBRARIES})
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L../buildwin")
endif ()


# On Android, PlugIns need a specific linkage set....
if (QT_ANDROID)
  # These libraries are needed to create PlugIns on Android.

  set(OCPN_Core_LIBRARIES
      # Presently, Android Plugins are built in the core tree, so the variables
      # {wxQT_BASE}, etc.
      # flow to this module from above.  If we want to build Android plugins
      # out-of-core, this will need improvement.
      # TODO This is pretty ugly, but there seems no way to avoid specifying a
      # full path in a cross build....
      /home/dsr/Projects/opencpn_sf/opencpn/build-opencpn-Android_for_armeabi_v7a_GCC_4_8_Qt_5_5_0-Debug/libopencpn.so
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_baseu-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_core-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_html-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_baseu_xml-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_qa-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_adv-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_aui-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_baseu_net-3.1-arm-linux-androideabi.a
      ${wxQt_Base}/${wxQt_Build}/lib/libwx_qtu_gl-3.1-arm-linux-androideabi.a
      ${Qt_Base}/android_armv7/lib/libQt5Core.so
      ${Qt_Base}/android_armv7/lib/libQt5OpenGL.so
      ${Qt_Base}/android_armv7/lib/libQt5Widgets.so
      ${Qt_Base}/android_armv7/lib/libQt5Gui.so
      ${Qt_Base}/android_armv7/lib/libQt5AndroidExtras.so
      # ${NDK_Base}/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi-v7a/libgnustl_shared.so
  )
endif (QT_ANDROID)
