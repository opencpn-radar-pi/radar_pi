#!/usr/bin/env bash

#
# Build for Raspbian in a docker container
#
# Bugs: The buster build is real slow: https://forums.balena.io/t/85743

set -xe

curl http://mirrordirector.raspbian.org/raspbian.public.key  | apt-key add -
curl http://archive.raspbian.org/raspbian.public.key  | apt-key add -
sudo apt-get -q update

sudo apt-get install -q devscripts equivs
sudo mk-build-deps -ir build-deps/control-raspbian
sudo apt-get -q --allow-unauthenticated install -f

# Temporary fix until 3.19 is available as a pypi package
# 3.19 is needed: https://gitlab.kitware.com/cmake/cmake/-/issues/20568
url='https://dl.cloudsmith.io/public/alec-leamas/opencpn-plugins-stable/deb/debian'
wget $url/pool/${OCPN_TARGET/-*/}/main/c/cm/cmake-data_3.19.3-0.1_all.deb
wget $url/pool/${OCPN_TARGET/-*/}/main/c/cm/cmake_3.19.3-0.1_armhf.deb
sudo apt install ./cmake_3.19.3-0.1_armhf.deb ./cmake-data_3.19.3-0.1_all.deb

rm -rf build; mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so

# Initiate python setup, install cryptography and ssh-client for git-push.
apt-get install -q python3-pip python3-cryptography openssh-client

# Latest pip 21.0.0 requires python 3.6+, we have only 3.5:
python3 -m pip install --user -q --force-reinstall pip==20.3.4 setuptools wheel

# Install cloudsmith-cli for upload script. 
python3 -m pip install --user -q cloudsmith-cli
