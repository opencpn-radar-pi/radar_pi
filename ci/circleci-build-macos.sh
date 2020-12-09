#!/usr/bin/env bash


#
# Build the  MacOS artifacts

set -xe

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

for pkg in cairo cmake gettext libarchive libexif python wget; do
    brew list --versions $pkg || brew install $pkg || brew install $pkg || :
    brew link --overwrite $pkg || brew install $pkg
done

if brew list --cask --versions packages; then
    version=$(pkg_version packages '--cask')
    sudo installer \
        -pkg /usr/local/Caskroom/packages/$version/packages/Packages.pkg \
        -target /
else
    brew install --cask packages
fi

wget -q https://download.opencpn.org/s/rwoCNGzx6G34tbC/download \
    -O /tmp/wx312B_opencpn50_macos109.tar.xz
tar -C /tmp -xJf /tmp/wx312B_opencpn50_macos109.tar.xz 

export MACOSX_DEPLOYMENT_TARGET=10.9

rm -rf build && mkdir build && cd build
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx312B_opencpn50_macos109/bin/wx-config \
  -DCMAKE_INSTALL_PREFIX="/" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  ..
make -j $(sysctl -n hw.physicalcpu) VERBOSE=1 tarball

make create-pkg

# Install cloudsmith needed by upload script
pip3 install cloudsmith-cli

# Required by git-push
pip3 install cryptography
