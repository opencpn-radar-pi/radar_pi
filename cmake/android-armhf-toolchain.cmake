set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 21)
set(CMAKE_ANDROID_ARCH_ABI armeabi-v7a)
if (DEFINED ENV{NDK_HOME})
  set(CMAKE_ANDROID_NDK $ENV{NDK_HOME})
else ()
  set(CMAKE_ANDROID_NDK /opt/android/ndk)
endif ()


set(wxQt_Build build_android_release_19_static_O3)
set(Qt_Build build_arm32_19_O3/qtbase)
