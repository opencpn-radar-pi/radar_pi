#!/usr/bin/env bash

#
# Upload the .tar.gz and .xml artifacts to cloudsmith
#
# Builds are uploaded the either the stable or the unstable
# repo. If there is a tag pointing to current commit it goes
# to stable, otherwise to unstable.
#
# If the environment variable CLOUDSMITH_STABLE_REPO exists it is
# used as the stable repo, defaulting to the hardcoded STABLE_REPO
# value. Likewise for CLOUDSMITH_UNSTABLE_REPO and UNSTABLE_REPO.
#

set -euo pipefail
set -x

STABLE_REPO=${CLOUDSMITH_STABLE_REPO:-'kees-verruijt/ocpn-plugins-stable'}
UNSTABLE_REPO=${CLOUDSMITH_UNSTABLE_REPO:-'kees-verruijt/ocpn-plugins-unstable'}

source $HOME/project/ci/commons.sh

if [ -z "$CIRCLECI" ]; then
    exit 0;
fi

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 0
fi

echo "Using \$CLOUDSMITH_API_KEY: ${CLOUDSMITH_API_KEY:0:4}..."

if pyenv versions 2>&1 >/dev/null; then
    pyenv global 3.7.0
    python -m pip install cloudsmith-cli
    pyenv rehash
elif dnf --version 2>&1 >/dev/null; then
    sudo dnf install python3-pip python3-setuptools
    sudo python3 -m pip install -q cloudsmith-cli
elif apt-get --version 2>&1 >/dev/null; then
    sudo apt-get install python3-pip python3-setuptools
    sudo python3 -m pip install -q cloudsmith-cli
else
    sudo -H python3 -m ensurepip
    sudo -H python3 -m pip install -q setuptools
    sudo -H python3 -m pip install -q cloudsmith-cli
fi

BUILD_ID=${CIRCLE_BUILD_NUM:-1}
commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
tag=$(git tag --contains HEAD)

xml=$(ls $HOME/project/build/*.xml)
tarball=$(ls $HOME/project/build/*.tar.gz)
tarball_basename=${tarball##*/}

source $HOME/project/build/pkg_version.sh
test -n "$tag" && VERSION="$tag" || VERSION="${VERSION}.${commit}"
test -n "$tag" && REPO="$STABLE_REPO" || REPO="$UNSTABLE_REPO"
tarball_name=radar-${PKG_TARGET}-${PKG_TARGET_VERSION}-tarball

sudo sed -i -e "s|@pkg_repo@|$REPO|"  $xml
sudo sed -i -e "s|@name@|$tarball_name|" $xml
sudo sed -i -e "s|@version@|$VERSION|" $xml
sudo sed -i -e "s|@filename@|$tarball_basename|" $xml

# Repack using gnu tar (cmake's is problematic) and add metadata.
sudo cp $xml metadata.xml
sudo chmod 666 $tarball
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
