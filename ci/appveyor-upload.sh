#!/usr/bin/env bash

set -euo pipefail
set -x

#
# Upload the .tar.gz and .xml artifacts to cloudsmith
#

source "../ci/cloudsmith-repo.sh"

python -m ensurepip
python -m pip install -q setuptools
python -m pip install -q cloudsmith-cli

commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
now=$(date --rfc-3339=seconds) || now=$(date)


BUILD_ID=${APPVEYOR_BUILD_NUMBER:-1}
commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
tag=$(git tag --contains HEAD)

xml=$(ls *.xml)
tarball=$(ls *.tar.gz)
tarball_basename=${tarball##*/}

source ../build/pkg_version.sh
test -n "$tag" && VERSION="$tag" || VERSION="${VERSION}.${commit}"
test -n "$tag" && REPO="$PROD_REPO" || REPO="$BETA_REPO"
tarball_name=radar-${PKG_TARGET}-${PKG_TARGET_VERSION}-tarball

# There is no sed available in git bash. This is nasty, but seems
# to work:
while read line; do
    line=${line/@pkg_repo@/$REPO}
    line=${line/@name@/$tarball_name}
    line=${line/@version@/$VERSION}
    line=${line/@filename@/$tarball_basename}
    echo $line
done < $xml > xml.tmp && cp xml.tmp $xml && rm xml.tmp

source ../ci/commons.sh
cp $xml metadata.xml
repack $tarball metadata.xml

_cloudsmith_options="push raw --republish --verbose"

cloudsmith ${_cloudsmith_options}                                               \
    --name radar-${PKG_TARGET}-${PKG_TARGET_VERSION}-metadata                   \
    --version ${VERSION}                                                        \
    --summary "radar opencpn plugin metadata for automatic installation"        \
    $REPO $xml

cloudsmith ${_cloudsmith_options}                                               \
    --name $tarball_name                                                        \
    --version ${VERSION}                                                        \
    --summary "radar opencpn plugin tarball for automatic installation"         \
    $REPO $tarball
