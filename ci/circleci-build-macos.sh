#!/usr/bin/env bash

#
# Build the  MacOS artifacts
#

# Fix broken ruby on the CircleCI image:
if [ -n "$CI" ]; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
fi

set -xe

set -o pipefail
for pkg in cairo cmake libarchive libexif wget; do
    brew list $pkg 2>&1 >/dev/null || brew install $pkg 2>&1 >/dev/null || brew upgrade $pkg
done
brew list python@2 2>&1 >/dev/null && brew unlink python@2
brew reinstall python3

wget -q http://opencpn.navnux.org/build_deps/wx312_opencpn50_macos109.tar.xz
tar xJf wx312_opencpn50_macos109.tar.xz -C /tmp
export PATH="/usr/local/opt/gettext/bin:$PATH"
echo 'export PATH="/usr/local/opt/gettext/bin:$PATH"' >> ~/.bash_profile

rm -rf build && mkdir build && cd build
cmake \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx312_opencpn50_macos109/bin/wx-config \
  -DwxWidgets_CONFIG_OPTIONS="--prefix=/tmp/wx312_opencpn50_macos109" \
  -DCMAKE_INSTALL_PREFIX= \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  "/" \
  ..
make -sj2
make package

wget -q http://opencpn.navnux.org/build_deps/Packages.dmg
hdiutil attach Packages.dmg
sudo installer -pkg "/Volumes/Packages 1.2.5/Install Packages.pkg" -target "/"
make create-pkg

