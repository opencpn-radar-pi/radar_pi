if (DEFINED _default_build_type)
  return ()
endif ()

# Set a default build type if none was specified
# https://blog.kitware.com/cmake-and-the-default-build-type/
set(_default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(_default_build_type "Debug")
endif()
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS
    "Setting build type to '${_default_build_type}' as none was specified."
  )
  set(CMAKE_BUILD_TYPE "${_default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set up OCPN_TARGET_TUPLE and update QT_ANDROID accordingly
set(OCPN_TARGET_TUPLE "" CACHE STRING
  "Target spec: \"platform;version;arch\""
)

string(TOLOWER "${OCPN_TARGET_TUPLE}" _lc_target)
if ("${_lc_target}" MATCHES "android*")
  set(QT_ANDROID ON)
  if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR
      "Android targets requires using a toolchain file. See INSTALL.md"
    )
  endif ()
else ()
  set(QT_ANDROID OFF)
  add_definitions(-D__OCPN_USE_CURL__)
endif ()
