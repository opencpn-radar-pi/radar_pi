#!/usr/bin/env bash

#
# Upload the .tar.gz and .xml artifacts to cloudsmith.
#


REPO='alec-leamas/opencpn-plugins-unstable'


if [ -z "$CIRCLECI" ]; then
    exit 0;
fi

branch=$(git symbolic-ref --short HEAD)
if [ "$branch" != 'master' ]; then
    echo "Not on master branch, skipping deployment."
    exit 0
fi

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 0
fi

echo "Using \$CLOUDSMITH_API_KEY: ${CLOUDSMITH_API_KEY:0:4}..."

set -xe

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
now=$(date --rfc-3339=seconds) || now=$(date)

tarball=$(ls $HOME/project/build/*.tar.gz)
xml=$(ls $HOME/project/build/*.xml)
sudo chmod 666 $xml
echo '<!--'" Date: $now Commit: $commit Build nr: $BUILD_ID -->" >> $xml

cloudsmith push raw --republish --no-wait-for-sync $REPO $tarball
cloudsmith push raw --republish --no-wait-for-sync $REPO $xml
