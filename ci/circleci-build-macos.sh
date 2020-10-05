#!/usr/bin/env bash

#
# Build the  MacOS artifacts

set -xe

#
# Check if the cache is with us. If not, re-install brew.
brew list --versions libexif || brew update-reset

for pkg in cairo cmake gettext libarchive libexif python wget; do
    brew list --versions $pkg || brew install $pkg || brew install $pkg || :
    brew link --overwrite $pkg || brew install $pkg
done

wget -q http://opencpn.navnux.org/build_deps/wx312_opencpn50_macos109.tar.xz
tar xJf wx312_opencpn50_macos109.tar.xz -C /tmp

export MACOSX_DEPLOYMENT_TARGET=10.9

rm -rf build && mkdir build && cd build
cmake \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx312_opencpn50_macos109/bin/wx-config \
  -DCMAKE_INSTALL_PREFIX="/" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  ..
make tarball

wget -q http://opencpn.navnux.org/build_deps/Packages.dmg
hdiutil attach Packages.dmg
sudo installer -pkg "/Volumes/Packages 1.2.5/Install Packages.pkg" -target "/"
make create-pkg

# Install cloudsmith needed by upload script
pip3 install cloudsmith-cli
