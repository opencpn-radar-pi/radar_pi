#!/usr/bin/env bash
#
# Build for Raspbian Bullseye in a docker container
#
# Intended as a temporary work-around for not being able to build on drone.io
# due to libseccomp problems with both host and guest OS i. e., #217
#
# Bugs: The build is real slow: https://forums.balena.io/t/85743

# Copyright (c) 2021 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set -xe

# Create build script
#
if [ -n "$TRAVIS_BUILD_DIR" ]; then
    ci_source="$TRAVIS_BUILD_DIR"
elif [ -d ~/project ]; then
    ci_source=~/project
else
    ci_source="$(pwd)"
fi

cat > $ci_source/build.sh << "EOF"

# The following two lines have already been executed in the docker image before upload
#sudo apt -y update
#sudo apt -y install devscripts equivs wget git lsb-release

echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
apt-get -qq update && DEBIAN_FRONTEND='noninteractive' TZ='America/New_York' apt-get -y --no-install-recommends install tzdata

sudo mk-build-deps  /ci-source/build-deps/control
sudo apt -y install ./opencpn-build-deps_1.0_all.deb
sudo apt-get -q --allow-unauthenticated install -f

#  cmake 3.20 was intalled in this docker image before uploading to dockerhub
#  Alternatively, cmake could be installed at runtime by something like this:

#CMAKE_VERSION=3.20.5-0kitware1ubuntu20.04.1
#wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc --no-check-certificate 2>/dev/null | apt-key add -
#apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
#apt-get update
#apt install cmake=$CMAKE_VERSION cmake-data=$CMAKE_VERSION

cd /ci-source
rm -rf build-ubuntu; mkdir build-ubuntu; cd build-ubuntu
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so
sudo chown --reference=.. .
EOF


cat $ci_source/build.sh

# Run script in docker image
#
if [ -n "$CI" ]; then
    sudo apt -q update
    sudo apt install qemu-user-static
fi
docker run --rm --privileged multiarch/qemu-user-static:register --reset || :
docker run --platform linux/arm/v7 --privileged \
    -e "OCPN_TARGET=$OCPN_TARGET" \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_BETA_REPO=$OCPN_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    -e "TRAVIS_BUILD_NUMBER=$TRAVIS_BUILD_NUMBER" \
    -v "$ci_source:/ci-source:rw" \
    opencpn/ubuntu-focal-armhf:v1 /bin/bash -xe /ci-source/build.sh
rm -f $ci_source/build.sh


# Install cloudsmith-cli (for upload) and cryptography (for git-push).
#
if pyenv versions &>/dev/null;  then
  pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1 \
      > $HOME/.python-version
fi

# Latest pip 21.0.0 requires python 3.7+, we have just 3.5:
python3 -m pip install -q --force-reinstall pip==20.3.4

# https://github.com/pyca/cryptography/issues/5753 -> cryptography < 3.4
python3 -m pip install -q --user cloudsmith-cli 'cryptography<3.4'

# python install scripts in ~/.local/bin, teach upload.sh to use in it's PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
