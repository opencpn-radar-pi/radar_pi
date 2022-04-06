# ~~~
# Summary:      Set up variables for xml metadata and upload scripts
# License:      GPLv3+
# Copyright (c) 2021 Alec Leamas
#
# Set up variables for configuration of xml metadata and upload scripts,
# all of which with a pkg_ prefix.
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

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
  COMMAND git tag --points-at HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE _git_tag
  RESULT_VARIABLE error_code
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT "$ENV{CIRCLE_BUILD_NUM}" STREQUAL "")
  set(_build_id "$ENV{CIRCLE_BUILD_NUM}")
  set(_pkg_build_info "$ENV{CIRCLE_BUILD_URL}")
elseif (NOT "$ENV{TRAVIS_BUILD_NUMBER}" STREQUAL "")
  set(_build_id "$ENV{TRAVIS_BUILD_NUMBER}")
  set(_pkg_build_info "$ENV{TRAVIS_BUILD_WEB_URL}")
elseif (NOT "$ENV{APPVEYOR_BUILD_NUMBER}" STREQUAL "")
  set(_build_id "$ENV{APPVEYOR_BUILD_NUMBER}")
  string(CONCAT _pkg_build_info
    "https://ci.appveyor.com/project"
    "/$ENV{APPVEYOR_ACCOUNT_NAME}/$ENV{APPVEYOR_PROJECT_SLUG}"
    "/builds/$ENV{APPVEYOR_BUILD_ID}"
  )
elseif (NOT "$ENV{DRONE_BUILD_NUMBER}" STREQUAL "")
  set(_build_id "$ENV{DRONE_BUILD_NUMBER}")
  set(_pkg_build_info
    "https://cloud.drone.io/$ENV{DRONE_REPO}/$ENV{DRONE_BUILD_NUMBER}"
  )
else ()
  string(TIMESTAMP _build_id "%y%m%d%H%M" UTC)
  cmake_host_system_information(RESULT _hostname QUERY HOSTNAME)
  set(_pkg_build_info "${_hostname} - ${_build_id}")
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

# pkg_build_info: Info about build host (link to log if available).
set(pkg_build_info ${_pkg_build_info})

# pkg_repo: Repository to use for upload
if ("${_git_tag}" STREQUAL "")
  set(pkg_repo "$ENV{CLOUDSMITH_UNSTABLE_REPO}")
  if ("${pkg_repo}" STREQUAL "")
    set(pkg_repo ${OCPN_TEST_REPO})
  endif ()
else ()
  string(TOLOWER  ${_git_tag}  _lc_git_tag)
  if (_lc_git_tag MATCHES "beta|rc")
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
# for untagged builds.
set(_pre_rel ${PKG_PRERELEASE})
if (NOT "${_pre_rel}" STREQUAL "" AND _pre_rel MATCHES "^[^-]")
  string(PREPEND _pre_rel "-")
endif ()
if ("${_git_tag}" STREQUAL "")
  set(pkg_semver "${PROJECT_VERSION}${_pre_rel}+${_build_id}.${_gitversion}")
else ()
  set(pkg_semver "${_git_tag}")
endif ()

# pkg_displayname: GUI name
if (ARCH MATCHES "arm64|aarch64")
  set(_display_arch "-A64")
elseif ("${_pkg_arch}" MATCHES "armhf")
  set(_display_arch "-A32")
endif()

if (NOT "${OCPN_WX_ABI}" STREQUAL "")
  set(_wx_abi ".${OCPN_WX_ABI}")
endif ()

if ("${_git_tag}" STREQUAL "")
  set(pkg_displayname "${PLUGIN_API_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}")
else ()
  set(pkg_displayname "${PLUGIN_API_NAME}-${_git_tag}")
endif ()
string(APPEND pkg_displayname
  "-${plugin_target}${_wx_abi}${_display_arch}-${plugin_target_version}"
)

# pkg_xmlname: XML metadata basename
set(pkg_xmlname ${pkg_displayname})

# pkg_tarname: Tarball basename
string(CONCAT pkg_tarname
  "${PLUGIN_API_NAME}-${pkg_semver}"
  "_${plugin_target}${_wx_abi}-${plugin_target_version}-${_pkg_arch}"
)

# pkg_tarball_url: Tarball location at cloudsmith
string(CONCAT pkg_tarball_url
  "https://dl.cloudsmith.io/public/${pkg_repo}/raw"
  "/names/${pkg_displayname}-tarball/versions/${pkg_semver}"
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
  if (NOT "${OCPN_WX_ABI}" STREQUAL "")
    set(pkg_target_arch "${pkg_target_arch}-${OCPN_WX_ABI}")
  endif ()
elseif ("${plugin_target}" MATCHES "ubuntu|raspbian|debian|mingw")
  set(pkg_target_arch "${plugin_target}-${ARCH}")
else ()
  set(pkg_target_arch "${plugin_target}")
endif ()

#cmake-format: on
