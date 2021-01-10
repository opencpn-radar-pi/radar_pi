#!/bin/sh  -xe
cd $(dirname $(readlink -fn $0))

#
# Actually build the Travis flatpak artifacts inside the Fedora container
#
set -xe

cd $TOPDIR
su -c "dnf install -y sudo cmake gcc-c++ flatpak-builder flatpak make tar"
flatpak remote-add --user --if-not-exists  flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.freedesktop.Sdk//18.08
flatpak install --user -y https://dl.flathub.org/repo/appstream/org.opencpn.OpenCPN.flatpakref
rm -rf build && mkdir build && cd build
cmake -DOCPN_FLATPAK=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make flatpak-build
make package

ls -lR
