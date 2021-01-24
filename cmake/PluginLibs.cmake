#
# Find and link general libraries to use: gettext, wxWidgets and OpenGL
#

find_package(Gettext REQUIRED)

set(wxWidgets_USE_DEBUG OFF)
set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_UNIVERSAL OFF)
set(wxWidgets_USE_STATIC OFF)

if (NOT QT_ANDROID)
  # QT_ANDROID is a cross-build, so the native FIND_PACKAGE(OpenGL) is
  # not useful.
  find_package(OpenGL)
  if (TARGET OpenGL::OpenGL)
    target_link_libraries(${PACKAGE_NAME} OpenGL::OpenGL)
  elseif (TARGET OpenGL::GL)
    target_link_libraries(${PACKAGE_NAME} OpenGL::GL)
  else ()
    message(WARNING "Cannot locate usable OpenGL libs and headers.")
  endif ()
  if (NOT OPENGL_GLU_FOUND)
    message(WARNING "Cannot find OpenGL GLU extension.")
  endif ()
  if (APPLE)
    # As of 3.19.2, cmake's FindOpenGL does not link to the directory
    # containing gl.h. cmake bug? Intended due to missing subdir GL/gl.h?
    find_path(GL_H_DIR NAMES gl.h)
    if (GL_H_DIR)
      target_include_directories(${PACKAGE_NAME} PRIVATE "${GL_H_DIR}")
    else ()
      message(WARNING "Cannot locate OpenGL header file gl.h")
    endif ()
  endif ()

  set(wxWidgets_USE_LIBS base core net xml html adv stc)
  set(BUILD_SHARED_LIBS TRUE)

  find_package(wxWidgets REQUIRED base core net xml html adv stc)
  if (MSYS)
    # This is just a hack. I think the bug is in FindwxWidgets.cmake
    string(
      REGEX REPLACE "/usr/local" "\\\\;C:/MinGW/msys/1.0/usr/local"
      wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS}
    )
  endif ()
  include(${wxWidgets_USE_FILE})	
endif ()

if (MINGW)
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
