#!/usr/bin/env bash


# Build the  MacOS artifacts


# Copyright (c) 2021 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set -x

# Load local environment if it exists i. e., this is a local build
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi

git submodule update --init opencpn-libs

# If applicable,  restore /usr/local from cache.
if [[ -n "$CI" && -f /tmp/local.cache.tar ]]; then
  sudo rm -rf /usr/local/*
  sudo tar -C /usr -xf /tmp/local.cache.tar
fi

# Set up build directory
rm -rf build-osx  && mkdir build-osx

# Create a log file.
exec > >(tee build-osx/build.log) 2>&1

export MACOSX_DEPLOYMENT_TARGET=10.10

# Return latest version of $1, optionally using option $2
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

#Install prebuilt dependencies
wget -q https://dl.cloudsmith.io/public/nohal/opencpn-plugins/raw/files/macos_deps_universal.tar.xz \
     -O /tmp/macos_deps_universal.tar.xz
sudo tar -C /usr/local -xJf /tmp/macos_deps_universal.tar.xz

export OPENSSL_ROOT_DIR='/usr/local'

# Build and package
cd build-osx
cmake \
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}" \
  -DCMAKE_INSTALL_PREFIX= \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET} \
  -DOCPN_TARGET_TUPLE="darwin-wx32;10;universal" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  ..

if [[ -z "$CI" ]]; then
    echo '$CI not found in environment, assuming local setup'
    echo "Complete build using 'cd build-osx; make tarball' or so."
    exit 0
fi

# nor-reproducible error on first invocation, seemingly tarball-conf-stamp
# is not created as required.
make VERBOSE=1 tarball || make VERBOSE=1 tarball

# Install cloudsmith needed by upload script
python3 -m pip install -q --user cloudsmith-cli

# Required by git-push
python3 -m pip install -q --user cryptography

# python3 installs in odd place not on PATH, teach upload.sh to use it:
pyvers=$(python3 --version | awk '{ print $2 }')
pyvers=$(echo $pyvers | sed -E 's/[\.][0-9]+$//')    # drop last .z in x.y.z
py_dir=$(ls -d  /Users/*/Library/Python/$pyvers/bin)
echo "export PATH=\$PATH:$py_dir" >> ~/.uploadrc

# Create the cached /usr/local archive
if [ -n "$CI"  ]; then
  tar -C /usr -cf /tmp/local.cache.tar  local
fi
