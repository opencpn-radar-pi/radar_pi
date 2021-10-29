#!/bin/sh  -xe

#
# Build the Android artifacts inside the circleci linux container
#
set -xe

sudo apt -q update
sudo apt install cmake git gettext

# Install cloudsmith-cli (for upload) and cryptography (for git-push)
sudo apt install python3-pip
python3 -m pip install --user -q --force-reinstall -q pip setuptools
sudo apt remove python3-six python3-colorama python3-urllib3
export LC_ALL=C.UTF-8  LANG=C.UTF-8
python3 -m pip install --user -q cloudsmith-cli cryptography

# Build tarball
builddir=build-android-64
test -d $builddir || mkdir $builddir
cd $builddir && rm -rf *

# Building using NDK requries a recent cmake, the packaged is too old
python3 -m pip install --user -q cmake

sudo ln -sf /opt/android/android-ndk-* /opt/android/ndk
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/android-aarch64-toolchain.cmake ..
make VERBOSE=1 android-aarch64
 
# Make sure that the upload script finds the files
cd ..; mv  $builddir build
