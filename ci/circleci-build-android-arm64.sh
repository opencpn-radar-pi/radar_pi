#!/bin/sh  -xe

#
# Build the Android artifacts inside the circleci linux container
#
set -xe

sudo apt -q update
sudo apt install cmake git gettext

# Install cloudsmith-cli (for upload) and cryptography (for git-push)
sudo apt install python3-pip
python3 -m pip install --user --force-reinstall -q pip setuptools
sudo apt remove python3-six python3-colorama python3-urllib3
export LC_ALL=C.UTF-8  LANG=C.UTF-8
python3 -m pip install --user cloudsmith-cli cryptography

# Build tarball
builddir=build-android-64
test -d $builddir || mkdir $builddir
cd $builddir && rm -rf *

tool_base="/opt/android/android-ndk-r21e/toolchains/llvm/prebuilt/linux-x86_64/"
cmake \
  -DOCPN_TARGET_TUPLE:STRING="Android-arm64;16;arm64" \
  -DwxQt_Build=build_android_release_64_static_O3 \
  -DQt_Build=build_arm64/qtbase \
  -DCMAKE_AR=${tool_base}/bin/aarch64-linux-android-ar \
  -DCMAKE_CXX_COMPILER=${tool_base}/bin/aarch64-linux-android21-clang++ \
  -DCMAKE_C_COMPILER=${tool_base}/bin/aarch64-linux-android21-clang \
  -DCMAKE_SYSROOT=${tool_base}/sysroot \
  -DCMAKE_INCLUDE_PATH=${tool_base}/sysroot/usr/include \
  -DCMAKE_LIBRARY_PATH=${tool_base}/sysroot/usr/lib/aarch64-linux-android/29 \
  .. 
make tarball

# Make sure that the upload script finds the files
cd ..; mv  $builddir build
