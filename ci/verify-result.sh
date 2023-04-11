#!/usr/bin/env bash
#
# Copyright (c) 2023 Kees Verruijt
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#

set -euo pipefail

OCPN_TARGET="${1:?Usage: verify-result.sh target-dir}"

# shellcheck disable=SC1090
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi

cd "${OCPN_TARGET}"
pkg_tarname="$(echo ./*.tar.gz)"
if [ ! -s "${pkg_tarname}" ] 
then
  echo "ERROR: Not exactly one .tar.gz file in $PWD"
  ls ./*.tar.gz
  exit 1
fi

pkg_xmlname="$(grep -l '<tarball-url>' ./*.xml)"
if [ ! -s "${pkg_xmlname}" ] 
then
  echo "ERROR: Not exactly one .xml file containing tarball-url in $PWD"
  ls ./*.xml
  exit 1
fi

if [[ ( ${pkg_tarname} =~ wx32 ) || ( ${pkg_tarname} =~ flatpak ) ]]
then 
  # Verify that any dll/dylib in the tar depends on wxWidgets 3.2

  EXTRACT_DIR="/tmp/extract"
  OLD_DIR="${PWD}"

  [ -d "${EXTRACT_DIR}" ] && rm -rf "${EXTRACT_DIR}"
  mkdir "${EXTRACT_DIR}"
  pushd "${EXTRACT_DIR}"
  tar xzf "${OLD_DIR}/${pkg_tarname}"
  find . -name '*.so' | while read -r i
  do
    if [ -f "${i}" ]
    then
      if objdump -p "${i}" | grep -q libwx >/dev/null
      then
        if objdump -p "${i}" | grep -q WXU_3.2 >/dev/null
        then
          echo "Verified that ${i} requires wxWidgets 3.2"
        else
          echo "ERROR: ${i} requires a version of wxWidgets is not version 3.2"
          objdump -p "${i}" | grep libwx
          exit 1
        fi
      else
        echo "${i} does not require wxWidgets"
      fi
    fi
  done
  find . -name '*.dylib' | while read -r i
  do
    if otool -L "${i}" | grep -q libwx > /dev/null
    then
      if otool -L "${i}" | grep -q 3.2 >/dev/null
      then
        echo "Verified that ${i} requires wxWidgets 3.2"
      else
        echo "ERROR: ${i} requires a version of wxWidgets is not version 3.2"
        otool -L "${i}" | grep libwx
        exit 1
      fi
    fi
  done
  popd
fi

sha_exe=sha256sum
which "${sha_exe}" || sha_exe='shasum -a 256'
test="$(echo 'test' | $sha_exe)"
if [ "$test" = "f2ca1bb6c7e907d06dafe4687e579fce76b37e4e93b7605022da52e6ccc26fd2  -" ]
then
  :
else
  echo "ERROR: No working shasum or sha256sum found"
  exit 1
fi

sha_xml="$(sed -n '/<tarball-checksum>/s! *<tarball-checksum> \([^ ]*\) </tarball-checksum> *!\1!p' < "${pkg_xmlname}")"
sha_file="$(${sha_exe} "${pkg_tarname}" | sed 's/ .*//')"
if [ "${sha_xml:-}" != "${sha_file:-}" ]
then
  echo "ERROR: MISMATCH IN CHECKSUM"
  echo "${sha_file} = ${pkg_tarname}"
  echo "${sha_xml} = ${pkg_xmlname}"
  cat "${pkg_xmlname}"
  exit 1
else
  echo "Verified checksum is ${sha_xml}"
fi
