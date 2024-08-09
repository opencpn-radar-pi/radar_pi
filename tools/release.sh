#!/bin/bash
#

set -euo pipefail

force=''
if [ "${1:-}" = "-f" ]
then
  force='--force'
  shift
fi

dryrun=''
if [ "${1:-}" = "-n" ]
then
  dryrun='echo'
  shift
fi

case "${1:-}" in
  beta)
    mode='beta'
    regexp='"beta.*"'
    ;;

  prod)
    mode='prod'
    regexp='""'
    ;;

  *)
    cat <<EOF
Usage: $0 [-f] [-n] {beta|prod}

To force tagging when this is the 2nd time you run this, use -f.
To do a dry run, use -n.

This script will:
  1. Check that the Plugin.cmake file contains either "beta..." or "" for the PKG_PRERELEASE variable.
  2. Check that no changes are outstanding.
  3. Tag the current commit as the release indicated by Plugin.cmake
  4. Push the tag and commit to Github.
EOF
    exit 1
esac

grep -q "PKG_PRERELEASE $regexp" Plugin.cmake ||
  (
    echo "ERROR: PKG_PRELEASE does not match a $mode release"
    exit 2
  )

[[ -n "$(git status -s)" ]] &&
  (
    echo "ERROR: git status not clean"
    exit 2
  )

#
# Extract the current release from Plugin.cmake
#
pkg_version="$(sed -n -e '/PKG_VERSION/s/.*VERSION *//p' Plugin.cmake | sed 's/)//')"
pkg_prerelease="$(awk -F'"' '/PKG_PRERELEASE/ { print $2 }' Plugin.cmake)"

tag="v${pkg_version}${pkg_prerelease:+-}${pkg_prerelease}"
echo "Tag: ${tag}"

if [ -n "${force}" ]
then
  # Remove any remote tag
  $dryrun git push --delete origin "$tag" || :
fi
$dryrun git tag -a ${force} -m"${tag} release" "$tag"
$dryrun git push --atomic origin master "$tag"


