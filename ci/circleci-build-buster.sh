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

# Read configuration and check if we should really build this
here=$(cd $(dirname $0); pwd -P)
source $here/../build-conf.rc
if [ "$oldstable_build_rate" -eq 0 ]; then exit 0; fi
if [ $((CIRCLE_BUILD_NUM % oldstable_build_rate)) -ne 0 ]; then
    exit 0
fi

# Load local environment if it exists i. e., this is a local build
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi
if [ -d /ci-source ]; then cd /ci-source; fi

git submodule update --init opencpn-libs

# Set up build directory and possibly a visible link in /
builddir=build-buster
test -d $builddir || sudo mkdir $builddir  && sudo rm -rf $builddir/*
sudo chmod 777 $builddir
if [ -w "/" ]; then    # Running as root i. e., a docker build
    if [ "$PWD" != "/" ]; then sudo ln -sf $PWD/$builddir /$builddir; fi
fi

# Create a log file.
exec > >(tee $builddir/build.log) 2>&1;

sudo apt -qq update || apt update
sudo apt-get -qq install devscripts equivs software-properties-common

mk-build-deps --root-cmd=sudo -ir build-deps/control
rm -f *changes  *buildinfo

sudo apt install -q \
    python3-pip python3-setuptools python3-dev python3-wheel \
    build-essential libssl-dev libffi-dev

python3 -m pip install --user --upgrade -q setuptools wheel pip
python3 -m pip install --user -q cloudsmith-cli cryptography cmake

cd $builddir
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make VERBOSE=1 tarball
ldd app/*/lib/opencpn/*.so

if [ -d /ci-source ]; then
    sudo chown --reference=/ci-source -R . ../cache || :
fi
sudo chmod --reference=.. .
