#!/usr/bin/env bash

#
# Upload the .tar.gz and .xml artifacts to cloudsmith
#

set -xe

REPO='alec-leamas/opencpn-plugins-unstable'

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 0
fi

python -m ensurepip
python -m pip install -q setuptools
python -m pip install -q cloudsmith-cli

commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
now=$(date --rfc-3339=seconds) || now=$(date)

tarball=$(ls *.tar.gz)
xml=$(ls *.xml)
echo '<!--'" Date: $now Commit: $commit Build nr: $BUILD_ID -->" >> $xml

cloudsmith push raw --republish --no-wait-for-sync $REPO $tarball
cloudsmith push raw --republish --no-wait-for-sync $REPO $xml
