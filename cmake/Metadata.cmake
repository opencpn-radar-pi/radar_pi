#
# Set up variables for configuration of xml metadata and upload scripts, all of
# which with a pkg_ prefix.
#

#cmake-format: off

if (DEFINED _pkg_metadata_done)
  return()
endif ()
set(_pkg_metadata_done 1)

include(GetArch)
include(PluginSetup)

# some helper vars (_ prefix)

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE _git_hash
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git tag --contains HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE _git_tag
  RESULT_VARIABLE error_code
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT "${error_code}" EQUAL 0)
  message(WARNING "Error code is: ${error_code}")
else ()
  message(STATUS "The tag is: ${_git_tag}")  
endif()


if (NOT "$ENV{CIRCLE_BUILD_NUM}" STREQUAL "")
  set(_build_id "$ENV{CIRCLE_BUILD_NUM}")
elseif (NOT "$ENV{TRAVIS_BUILD_NUMBER}" STREQUAL "")
  set(_build_id "$ENV{TRAVIS_BUILD_NUMBER}")
elseif (NOT "$ENV{APPVEYOR_BUILD_NUMBER}" STREQUAL "")
  set(_build_id "$ENV{APPVEYOR_BUILD_NUMBER}")
else ()
  string(TIMESTAMP _build_id "%y%m%d%H%M" UTC)
endif ()

if ("${_git_tag}" STREQUAL "")
  set(_gitversion "${_git_hash}")
else ()
  set(_gitversion "${_git_tag}")
endif ()

if (WIN32)
  set(_pkg_arch "win32")
else ()
  set(_pkg_arch "${ARCH}")
endif ()

# pkg_repo: Repository to use for upload
if ("${_git_tag}" STREQUAL "")
  set(pkg_repo "$ENV{CLOUDSMITH_UNSTABLE_REPO}")
  if ("${pkg_repo}" STREQUAL "")
    set(pkg_repo ${OCPN_TEST_REPO})
  endif ()
else ()
  string(TOLOWER  ${_git_tag}  _lc_git_tag)
  if (_lc_git_tag MATCHES "beta")
    set(pkg_repo "$ENV{CLOUDSMITH_BETA_REPO}")
    if ("${pkg_repo}" STREQUAL "")
      set(pkg_repo ${OCPN_BETA_REPO})
    endif ()
  else ()
    set(pkg_repo "$ENV{CLOUDSMITH_STABLE_REPO}")
    if ("${pkg_repo}" STREQUAL "")
      set(pkg_repo ${OCPN_RELEASE_REPO})
    endif ()
  endif()
endif ()

# Make sure repo is displayed even if builders hides environment variables.
string(REGEX REPLACE "([a-zA-Z0-9/-])" "\\1 " pkg_repo_display  ${pkg_repo})
message(STATUS "Selected upload repository: ${pkg_repo_display}")

# pkg_semver: Complete version including pre-release tag and build info
set(_pre_rel ${PKG_PRERELEASE})
if (_pre_rel MATCHES "^[^-]")
  string(PREPEND _pre_rel "-")
endif ()
set(pkg_semver "${PROJECT_VERSION}${_pre_rel}+${_build_id}.${_gitversion}")

# pkg_displayname: Used for xml metadata and GUI name
if (ARCH MATCHES "arm64|aarch64")
  set(_display_arch "-A64")
endif()
string(CONCAT pkg_displayname
  "${PLUGIN_API_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}"
  "-${plugin_target}${_display_arch}-${plugin_target_version}"
)

# pkg_tarname: Tarball basename
string(CONCAT pkg_tarname 
  "${PLUGIN_API_NAME}-${pkg_semver}_"
  "${plugin_target}-${plugin_target_version}-${_pkg_arch}"
)

# pkg_tarball_url: Tarball location at cloudsmith
string(CONCAT pkg_tarball_url
  "https://dl.cloudsmith.io/public/${pkg_repo}/raw"
  "/names/${pkg_displayname}-tarball" "/versions/${pkg_semver}"
  "/${pkg_tarname}.tar.gz"
)

# pkg_python: python command
find_program(PY_WRAPPER py) # (at least) appveyor build machines
find_program(PYTHON3 python3)
if (PY_WRAPPER)
  set(pkg_python ${PY_WRAPPER})
elseif (PYTHON3)
  set(pkg_python ${PYTHON3})
else ()
  set(pkg_python python)
endif ()

# pkg_target_arch: os + optional -arch suffix. See: Opencpn bug #2003
if ("${BUILD_TYPE}" STREQUAL "flatpak")
  set(pkg_target_arch "flatpak-${ARCH}")
elseif ("${plugin_target}" MATCHES "ubuntu|raspbian|debian|mingw")
  set(pkg_target_arch "${plugin_target}-${ARCH}")
else ()
  set(pkg_target_arch "${plugin_target}")
endif ()

#cmake-format: on
