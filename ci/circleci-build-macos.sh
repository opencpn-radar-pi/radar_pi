#!/usr/bin/env bash


# Build the  MacOS artifacts


# Copyright (c) 2021 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set -xe

# Load local environment if it exists i. e., this is a local build
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi

git submodule update --init opencpn-libs

# Set up build directory
if [ -n "$TRAVIS_BUILD_DIR" ]; then cd $TRAVIS_BUILD_DIR; fi
rm -rf build-osx  && mkdir build-osx

# Create a log file.
exec > >(tee build-osx/build.log) 2>&1

export MACOSX_DEPLOYMENT_TARGET=10.10

# Return latest version of $1, optiomally using option $2
pkg_version() { brew list --versions $2 $1 | tail -1 | awk '{print $2}'; }

#
# Check if the cache is with us. If not, re-install brew.
brew list --versions libexif || brew update-reset

# Install packaged dependencies
here=$(cd "$(dirname "$0")"; pwd)
for pkg in $(sed '/#/d' < $here/../build-deps/macos-deps);  do
    brew list --versions $pkg || brew install $pkg || brew install $pkg || :
    brew link --overwrite $pkg || brew install $pkg
done

export OPENSSL_ROOT_DIR='/usr/local/opt/openssl'

# Install the pre-built wxWidgets package

wget -q https://download.opencpn.org/s/MCiRiq4fJcKD56r/download \
    -O /tmp/wx315_opencpn50_macos1010.tar.xz
tar -C /tmp -xJf /tmp/wx315_opencpn50_macos1010.tar.xz

# Build and package
cd build-osx
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx315_opencpn50_macos1010/bin/wx-config \
  -DwxWidgets_CONFIG_OPTIONS="--prefix=/tmp/wx315_opencpn50_macos1010" \
  -DCMAKE_INSTALL_PREFIX= \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 \
  ..

if [[ -z "$CI" ]]; then
    echo '$CI not found in environment, assuming local setup'
    echo "Complete build using 'cd build-osx; make tarball' or so."
    exit 0
fi

make VERBOSE=1 tarball

# Install cloudsmith needed by upload script
python3 -m pip install -q --user cloudsmith-cli

# Required by git-push
python3 -m pip install -q --user cryptography

# python3 installs in odd place not on PATH, teach upload.sh to use it:
pyvers=$(python3 --version | awk '{ print $2 }')
pyvers=$(echo $pyvers | sed -E 's/[\.][0-9]+$//')    # drop last .z in x.y.z
py_dir=$(ls -d  /Users/*/Library/Python/$pyvers/bin)
echo "export PATH=\$PATH:$py_dir" >> ~/.uploadrc
