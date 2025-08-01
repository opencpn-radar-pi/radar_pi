#!/bin/bash  -xe

#
# Build the Android artifacts inside the circleci linux container
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
if [ "$android_build_rate" -eq 0 ]; then exit 0; fi
if [ $((CIRCLE_BUILD_NUM % android_build_rate)) -ne 0 ]; then
    exit 0
fi

# Load local environment if it exists i. e., this is a local build
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi
if [ -d /ci-source ]; then cd /ci-source; fi

# Handle possible outdated key for google packages, see #487
curl https://packages.cloud.google.com/apt/doc/apt-key.gpg \
    | sudo apt-key add -

git submodule update --init opencpn-libs

# Set up build directory and a visible link in /
builddir=build-$OCPN_TARGET
test -d $builddir || sudo mkdir $builddir && sudo rm -rf $builddir/*
sudo chmod 777 $builddir
if [ "$PWD" != "/"  ]; then sudo ln -sf $PWD/$builddir /$builddir; fi

# Create a log file.
exec > >(tee $builddir/build.log) 2>&1

# The local container needs to access the cache directory
test -d cache || sudo mkdir cache
test -w cache || sudo chmod -R go+w cache || :
 

sudo apt -q update
sudo apt install -q cmake git gettext

# Install cloudsmith-cli (for upload) and cryptography (for git-push)
sudo apt install python3-pip
python3 -m pip install --user --force-reinstall -q pip setuptools
sudo apt remove python3-six python3-colorama python3-urllib3
export LC_ALL=C.UTF-8  LANG=C.UTF-8
python3 -m pip install --user -q cloudsmith-cli cryptography

# Building using NDK requires a recent cmake, the packaged is too old
python3 -m pip install --user -q cmake

# Build tarball
cd $builddir

last_ndk=$(ls -d /home/circleci/android-sdk/ndk/* | tail -1)
test -d /opt/android || sudo mkdir -p /opt/android
sudo ln -sf $last_ndk /opt/android/ndk
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/$OCPN_TARGET-toolchain.cmake ..
make VERBOSE=1

if [ -d /ci-source ]; then sudo chown --reference=/ci-source -R . ../cache; fi
sudo chmod --reference=.. .
