#!/bin/sh  -xe

#
# Build the Android artifacts inside the circleci linux container
#
set -xe

sudo apt -q update
sudo apt install -q cmake git gettext

# Install cloudsmith-cli (for upload) and cryptography (for git-push)
sudo apt install python3-pip
python3 -m pip install --user --force-reinstall -q pip setuptools
sudo apt remove python3-six python3-colorama python3-urllib3
export LC_ALL=C.UTF-8  LANG=C.UTF-8
python3 -m pip install --user -q cloudsmith-cli cryptography

# Building using NDK requries a recent cmake, the packaged is too old
python3 -m pip install --user -q cmake

# Build tarball
test -d build || mkdir build
cd build && rm -rf *

sudo ln -sf /opt/android/android-ndk-* /opt/android/ndk
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/$OCPN_TARGET-toolchain.cmake ..
make VERBOSE=1 $OCPN_TARGET
