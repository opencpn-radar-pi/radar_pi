#!/usr/bin/env bash

#
# Upload the .tar.gz and .xml artifacts to cloudsmith
#

set -xe

UNSTABLE_REPO=${CLOUDSMITH_UNSTABLE_REPO:-'david-register/ocpn-plugins-unstable'}
STABLE_REPO=${CLOUDSMITH_STABLE_REPO:-'david-register/ocpn-plugins-stable'}

source $HOME/project/ci/commons.sh

if [ -z "$CIRCLECI" ]; then
    exit 0;
fi

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 0
fi

BUILD_ID=${CIRCLE_BUILD_NUM:-1}
commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
tag=$(git tag --contains HEAD)

xml=$(ls $HOME/project/build/*.xml)
tarball=$(ls $HOME/project/build/*.tar.gz)
tarball_basename=${tarball##*/}



source $HOME/project/build/pkg_version.sh
test -n "$tag" && VERSION="$tag" || VERSION="${VERSION}+${BUILD_ID}.${commit}"
test -n "$tag" && REPO="$STABLE_REPO" || REPO="$UNSTABLE_REPO"
# extract the project name for a filename.  e.g. oernc-pi... sets PROJECT to  "oernc"
PROJECT=${tarball_basename%%_pi*}

# split into pkg_name, pkg_version and pkg_tail
split_pkg $tarball_basename
tarball_name="$pkg_name-$pkg_version-${VERSION}_$pkg_tail"

sudo sed -i -e "s|=pkg_repo=|$REPO|" $xml
sudo sed -i -e "s|=name=|$tarball_name|" $xml
sudo sed -i -e "s|=version=|$VERSION|" $xml
sudo sed -i -e "s|=filename=|$tarball_basename|" $xml

# Repack using gnu tar (cmake's is problematic) and add metadata.
cp $xml metadata.xml
sudo chmod 666 $tarball
repack $tarball metadata.xml


cloudsmith push raw --republish --no-wait-for-sync \
    --name ${PROJECT}-${PKG_TARGET}-${PKG_TARGET_VERSION}-metadata \
    --version ${VERSION} \
    --summary "opencpn plugin metadata for automatic installation" \
    $REPO $xml

cloudsmith push raw --republish --no-wait-for-sync \
    --name $tarball_name \
    --version ${VERSION} \
    --summary "opencpn plugin tarball for automatic installation" \
    $REPO $tarball
