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
curl http://mirrordirector.raspbian.org/raspbian.public.key  | apt-key add -
curl http://archive.raspbian.org/raspbian.public.key  | apt-key add -
sudo apt -q update

sudo apt install devscripts equivs wget git lsb-release
sudo mk-build-deps -ir /ci-source/build-deps/control
sudo apt-get -q --allow-unauthenticated install -f

# Temporary fix until 3.19 is available as a pypi package
# 3.19 is needed: https://gitlab.kitware.com/cmake/cmake/-/issues/20568
url='https://dl.cloudsmith.io/public/alec-leamas/opencpn-plugins-stable/deb/debian'
wget $url/pool/bullseye/main/c/cm/cmake-data_3.20.5-0.1/cmake-data_3.20.5-0.1_all.deb
wget $url/pool/bullseye/main/c/cm/cmake_3.20.5-0.1/cmake_3.20.5-0.1_armhf.deb
sudo apt install ./cmake_3.*-0.1_armhf.deb ./cmake-data_3.*-0.1_all.deb

cd /ci-source
rm -rf build-raspbian; mkdir build-raspbian; cd build-raspbian
cmake -DCMAKE_BUILD_TYPE=debug ..
make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so
sudo chmod --reference=.. .
EOF


# Run script in docker image
#
if [ -n "$CI" ]; then
    sudo apt -q update
    sudo apt install qemu-user-static
fi
docker run --rm --privileged multiarch/qemu-user-static:register --reset || :
docker run --privileged \
    -e "OCPN_TARGET=$OCPN_TARGET" \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_BETA_REPO=$OCPN_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    -e "TRAVIS_BUILD_NUMBER=$TRAVIS_BUILD_NUMBER" \
    -v "$ci_source:/ci-source:rw" \
    balenalib/raspberry-pi-debian:bullseye /bin/bash -xe /ci-source/build.sh
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
