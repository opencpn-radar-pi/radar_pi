#!/bin/sh  -xe

#
# Actually build the Travis mingw artifacts inside teh Fedora container
#
set -xe

df -h
cd /opencpn-ci
su -c "dnf install -y sudo dnf-plugins-core"
sudo dnf builddep  -y ci/opencpn-fedora.spec
rm -rf build; mkdir build; cd build
cmake ..
make -j2
make package
