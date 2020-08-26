#!/usr/bin/env bash

#
# Build the Travis MacOS artifacts
#

set -xe

# Install updated ruby, system's is too old. Installations
# fails, but seems to work anyway at last.
brew install ruby || brew install ruby || :
export PATH="/usr/local/opt/ruby/bin:$PATH"

for pkg in cairo cmake libarchive libexif python3 wget; do
    brew list $pkg >/dev/null 2>&1 || brew install $pkg
done

wget http://opencpn.navnux.org/build_deps/wx312_opencpn50_macos109.tar.xz
tar xJf wx312_opencpn50_macos109.tar.xz -C /tmp
export PATH="/usr/local/opt/gettext/bin:$PATH"

rm -rf build && mkdir build && cd build
cmake \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx312_opencpn50_macos109/bin/wx-config \
  -DwxWidgets_CONFIG_OPTIONS="--prefix=/tmp/wx312_opencpn50_macos109" \
  -DCMAKE_INSTALL_PREFIX= "/" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  ..
make -sj2
make package
make repack-tarball

# Install cloudsmith-cli, required by upload script.
sudo -H python3 -m ensurepip
sudo -H python3 -m pip install -q setuptools
sudo -H python3 -m pip install -q cloudsmith-cli
