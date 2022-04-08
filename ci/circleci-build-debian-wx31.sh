#!/usr/bin/env bash

#
# Build the Debian artifacts
#

# Copyright (c) 2021 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set -xe

# Load local environment if it exists i. e., this is a local build
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi
if [ -d /ci-source ]; then cd /ci-source; fi

git submodule update --init opencpn-libs

# Set up build directory and a visible link in /
builddir=build-$OCPN_TARGET
test -d $builddir || sudo mkdir $builddir  && sudo rm -rf $builddir/*
sudo chmod 777 $builddir
if [ "$PWD" != "/"  ]; then sudo ln -sf $PWD/$builddir /$builddir; fi

# Create a log file.
exec > >(tee $builddir/build.log) 2>&1;

sudo add-apt-repository ppa:leamas-alec/wxwidgets
sudo apt -qq update || apt update
sudo apt-get -qq install devscripts equivs software-properties-common

sudo mk-build-deps -ir build-deps/control
sudo apt-get -q --allow-unauthenticated install -f

sudo apt install -q \
    python3-pip python3-setuptools python3-dev python3-wheel \
    build-essential libssl-dev libffi-dev

python3 -m pip install --user --upgrade -q setuptools wheel pip
python3 -m pip install --user -q cloudsmith-cli cryptography cmake

cd $builddir

cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DOCPN_TARGET_TUPLE="ubuntu-wx315;20.04;x86_64" \
    ..
make VERBOSE=1 tarball
ldd app/*/lib/opencpn/*.so
if [ -d /ci-source ]; then
    sudo chown --reference=/ci-source -R . ../cache || :
fi
sudo chmod --reference=.. .
