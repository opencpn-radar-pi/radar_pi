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

if (NOT "$ENV{CIRCLE_BUILD_NUM}" STREQUAL "")
  set(_build_id "$ENV{CIRCLE_BUILD_NUM}")
elseif (NOT "$ENV{TRAVIS_BUILD_NUMBER}" STREQUAL "")
  set(_build_id "$ENV{TRAVIS_BUILD_NUMBER}")
elseif (NOT "$ENV{APPVEYOR_BUILD_NUMBER}" STREQUAL "")
  set(_build_id "$ENV{APPVEYOR_BUILD_NUMBER}")
else ()
  string(TIMESTAMP _build_id "%y%m%d%H%M" UTC)
endif ()

if ("${GIT_TAG}" STREQUAL "")
  set(_gitversion "${GIT_HASH}")
else ()
  set(_gitversion "${GIT_TAG}")
endif ()

if (WIN32)
  set(_pkg_arch "win32")
else ()
  set(_pkg_arch "${ARCH}")
endif ()

# pkg_repo: Repository to use for upload
if ("${GIT_TAG}" STREQUAL "")
  if ("$ENV{CLOUDSMITH_UNSTABLE_REPO}" STREQUAL "")
    set(pkg_repo ${OCPN_UNSTABLE_REPO})
  else ()
    set(pkg_repo "$ENV{CLOUDSMITH_UNSTABLE_REPO}")
  endif ()
else ()
  if ("$ENV{CLOUDSMITH_STABLE_REPO}" STREQUAL "")
    set(pkg_repo ${OCPN_STABLE_REPO})
  else ()
    set(pkg_repo "$ENV{CLOUDSMITH_STABLE_REPO}")
  endif ()
endif ()

# pkg_semver: Complete version including build info
set(pkg_semver "${PACKAGE_VERSION}+${_build_id}.${_gitversion}")

# pkg_displayname: Used for xml metadata and GUI name
set(pkg_displayname "${PLUGIN_API_NAME}-${PKG_TARGET}-${PKG_TARGET_VERSION}")

# pkg_tarname: Tarball basename
string(CONCAT pkg_tarname 
  "${PLUGIN_API_NAME}-${pkg_semver}_"
  "${PKG_TARGET}-${PKG_TARGET_VERSION}-${_pkg_arch}"
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
  set(pkg_python py)
elseif (PYTHON3)
  set(pkg_python python3)
else ()
  set(pkg_python python)
endif ()

# pkg_target_arch: os + optional -arch suffix. See: Opencpn bug #2003
if ("${BUILD_TYPE}" STREQUAL "flatpak")
  set(pkg_target_arch "flatpak-${ARCH}")
elseif ("${PKG_TARGET}" STREQUAL "ubuntu")
  set(pkg_target_arch "ubuntu-${ARCH}")
elseif ("${PKG_TARGET}" STREQUAL "raspbian")
  set(pkg_target_arch "raspbian-${ARCH}")
elseif ("${PKG_TARGET}" STREQUAL "debian")
  set(pkg_target_arch "debian-${ARCH}")
else ()
  set(pkg_target_arch "${PKG_TARGET}")
endif ()

#cmake-format: on
