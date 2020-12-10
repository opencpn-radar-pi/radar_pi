#!/usr/bin/env bash

#
# Build the Debian artifacts
#
set -xe
sudo apt -qq update || apt update
sudo apt-get -q install  devscripts equivs

mkdir  build
cd build
sudo mk-build-deps -ir ../ci/control
sudo apt-get -q --allow-unauthenticated install -f

if [ -n "$BUILD_GTK3" ]; then
    sudo update-alternatives --set wx-config \
        /usr/lib/*-linux-*/wx/config/gtk3-unicode-3.0
fi

cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 tarball

sudo apt-get install \
    python3-pip python3-setuptools python3-dev python3-wheel \
    build-essential libssl-dev libffi-dev 

python3 -m pip install --user --upgrade pip
python3 -m pip install --user -q cloudsmith-cli
python3 -m pip install --user -q cryptography
