#!/usr/bin/env bash


#
# Build the  MacOS artifacts

set -xe

# Return latest version of $1, optiomally using option $2
pkg_version() { brew list --versions $2 $1 | tail -1 | awk '{print $2}'; }

#
# Check if the cache is with us. If not, re-install brew.
brew list --versions libexif || brew update-reset


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
  -DwxWidgets_CONFIG_OPTIONS="--prefix=/tmp/wx312B_opencpn50_macos109" \
  -DCMAKE_INSTALL_PREFIX= \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  ..
make -j $(sysctl -n hw.physicalcpu) VERBOSE=1 tarball

make create-pkg

# Install cloudsmith needed by upload script
python3 -m pip install --user cloudsmith-cli

# Required by git-push
python3 -m pip install --user cryptography

# python3 installs in odd place not on PATH:
pyvers=$(python3 --version | awk '{ print $2 }')
pyvers=$(echo $pyvers | sed -E 's/[\.][0-9]+$//')    # drop last .z in x.y.z
echo "export PATH=\$PATH:/Users/distiller/Library/Python/$pyvers/bin" \
    >> ~/.uploadrc
