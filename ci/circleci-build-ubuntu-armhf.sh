#!/usr/bin/env bash
#
# Build for Ubuntu armhf in a docker container
#
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

cd $ci_source
git submodule update --init opencpn-libs

cat > $ci_source/build.sh << "EOF"

# The  docker images are updated and have installed devscripts and equivs
# i. e., what is required for mk-build-deps.

apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 6AF7F09730B3F0A4
export DEBIAN_FRONTEND=noninteractive
apt -q update
mk-build-deps  /ci-source/build-deps/control
apt -y install ./opencpn-build-deps_1.0_all.deb
sudo apt-get -q --allow-unauthenticated install -f

# cmake 3.20/3.22 is installed in the docker images before uploading to
# dockerhub. Alternatively, cmake could be installed at configure time
# as described in  https://apt.kitware.com/

cd /ci-source
rm -rf build-ubuntu; mkdir build-ubuntu; cd build-ubuntu
cmake -DCMAKE_BUILD_TYPE=Release -DOCPN_TARGET_TUPLE="@TARGET_TUPLE@" ..
make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so
sudo chown --reference=.. .
EOF

sed -i "s/@TARGET_TUPLE@/$TARGET_TUPLE/" $ci_source/build.sh


# Run script in docker image
#
if [ -n "$CI" ]; then
    sudo apt -q update
    sudo apt install qemu-user-static
fi
docker run --rm --privileged multiarch/qemu-user-static:register --reset || :
docker run --platform linux/arm/v7 --privileged \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_BETA_REPO=$OCPN_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    -e "TRAVIS_BUILD_NUMBER=$TRAVIS_BUILD_NUMBER" \
    -v "$ci_source:/ci-source:rw" \
    ${DOCKER_IMAGE:-opencpn/ubuntu-focal-armhf:v1} /bin/bash -xe /ci-source/build.sh
rm -f $ci_source/build.sh


# Install cloudsmith-cli (for upload) and cryptography (for git-push).
#
if pyenv versions &>/dev/null;  then
    pyenv versions | tr -d '*' | awk '{print $1}' | tail -1 \
        > $HOME/.python-version
fi
python3 -m pip install -q --user cloudsmith-cli cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use in it's PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
