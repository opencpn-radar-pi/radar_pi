#!/bin/sh  -xe
cd $(dirname $(readlink -fn $0))

#
# Actually build the mingw artifacts inside the Fedora container
#
set -xe

cd $TOPDIR

su -c "dnf install -q -y sudo cmake gcc-c++ flatpak-builder flatpak make tar"
flatpak remote-add --user --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.opencpn.OpenCPN > /dev/null
flatpak install --user -y flathub org.freedesktop.Sdk//18.08  >/dev/null
sed -i '/^runtime-version/s/:.*/: stable/' \
    flatpak/org.opencpn.OpenCPN.Plugin.shipdriver.yaml
rm -rf build && mkdir build && cd build
cmake -DOCPN_FLATPAK=ON ..
make flatpak-build
make flatpak-pkg
make repack-tarball
