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

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make tarball

#sudo apt-get install python3-pip python3-setuptools
#sudo python3 -m pip install -q cloudsmith-cli

wget https://files.pythonhosted.org/packages/59/4a/dbd1cc20156d0a9c10e2b085b8c14480166b0f765086ba2d9150495ec626/cloudsmith_cli-0.23.0-py2.py3-none-any.whl
sudo pip3 install cloudsmith_cli-0.23.0-py2.py3-none-any.whl
