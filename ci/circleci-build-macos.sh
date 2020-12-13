#!/usr/bin/env bash

#
# Build the  MacOS artifacts 
#

set -euo pipefail
set -x

export MACOSX_DEPLOYMENT_TARGET=10.9

# Return latest installed brew version of given package
pkg_version() { brew list --versions "${@}" | tail -1 | awk '{print $2}'; }

#
# Check if the cache is with us. If not, re-install brew.
brew list --versions libexif || {
    brew update-reset
    # As indicated by warning message in CircleCI build log:
    git -C "/usr/local/Homebrew/Library/Taps/homebrew/homebrew-core" \
        fetch --unshallow
    git -C "/usr/local/Homebrew/Library/Taps/homebrew/homebrew-cask" \
        fetch --unshallow
}

for pkg in cairo cmake libarchive libexif pixman python3 wget xz; do
    brew list --versions $pkg || brew install $pkg || brew install $pkg || :
    brew link --overwrite $pkg || :
done

# Make sure cmake finds libarchive
version=$(pkg_version libarchive)
pushd /usr/local/include
    ln -sf /usr/local/Cellar/libarchive/$version/include/archive.h .
    ln -sf /usr/local/Cellar/libarchive/$version/include/archive_entry.h .
    cd ../lib
    ln -sf  /usr/local/Cellar/libarchive/$version/lib/libarchive.13.dylib .
    ln -sf  /usr/local/Cellar/libarchive/$version/lib/libarchive.dylib .
popd

if brew list --cask --versions packages; then
    version=$(pkg_version '--cask' packages)
    sudo installer \
        -pkg /usr/local/Caskroom/packages/$version/packages/Packages.pkg \
        -target /
else
    brew install --cask packages
fi


wget -q https://download.opencpn.org/s/rwoCNGzx6G34tbC/download \
    -O /tmp/wx312B_opencpn50_macos109.tar.xz
tar -C /tmp -xJf /tmp/wx312B_opencpn50_macos109.tar.xz 
export PATH="/usr/local/opt/gettext/bin:$PATH"
echo 'export PATH="/usr/local/opt/gettext/bin:$PATH"' >> ~/.bash_profile

rm -rf build && mkdir build && cd build
cmake -DOCPN_USE_LIBCPP=ON \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx312B_opencpn50_macos109/bin/wx-config \
  -DwxWidgets_CONFIG_OPTIONS="--prefix=/tmp/wx312B_opencpn50_macos109" \
  -DCMAKE_INSTALL_PREFIX=/tmp/opencpn -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -sj2
make package
ls -lR
