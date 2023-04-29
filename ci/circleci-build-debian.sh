#!/usr/bin/env bash

#
# Build the Debian artifacts
#

# Copyright (c) 2021 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


# Remove existing wx30 packages
function remove_wx30() {
  sudo apt remove \
      libwxsvg3 \
      wx3.0-i18n \
      wx-common \
      libwxgtk3.0-gtk3-0v5 \
      libwxbase3.0-0v5 \
      libwxsvg3 wx3.0-i18n \
      wx-common \
      libwxgtk3.0-gtk3-0v5 \
      libwxbase3.0-0v5 wx3.0-headers \
      libwxsvg3 \
      libwxsvg-dev
}

# Install generated packages
function install_wx32() {
  test -d /usr/local/pkg || sudo mkdir /usr/local/pkg
  sudo chmod a+w /usr/local/pkg
  repo="https://dl.cloudsmith.io/public/alec-leamas/wxwidgets-32"
  head="deb/debian/pool/bullseye/main"
  vers="3.2.2+dfsg-1~bpo11+1"
  pushd /usr/local/pkg
  wget -q $repo/$head/w/wx/wx-common_${vers}/wx-common_${vers}_amd64.deb
  wget -q $repo/$head/w/wx/wx3.2-i18n_${vers}/wx3.2-i18n_${vers}_all.deb
  wget -q $repo/$head/w/wx/wx3.2-headers_${vers}/wx3.2-headers_${vers}_all.deb
  wget -q $repo/$head/l/li/libwxgtk-webview3.2-dev_${vers}/libwxgtk-webview3.2-dev_${vers}_amd64.deb
  wget -q $repo/$head/l/li/libwxgtk-webview3.2-1_${vers}/libwxgtk-webview3.2-1_${vers}_amd64.deb
  wget -q $repo/$head/l/li/libwxgtk-media3.2-dev_${vers}/libwxgtk-media3.2-dev_${vers}_amd64.deb
  wget -q $repo/$head/l/li/libwxgtk3.2-dev_${vers}/libwxgtk3.2-dev_${vers}_amd64.deb
  wget -q $repo/$head/l/li/libwxgtk3.2-1_${vers}/libwxgtk3.2-1_${vers}_amd64.deb
  wget -q $repo/$head/l/li/libwxgtk-gl3.2-1_${vers}/libwxgtk-gl3.2-1_${vers}_amd64.deb
  wget -q $repo/$head/l/li/libwxbase3.2-1_${vers}/libwxbase3.2-1_${vers}_amd64.deb
  wget -q $repo/$head/l/li/libwxgtk-media3.2-1_${vers}/libwxgtk-media3.2-1_${vers}_amd64.deb

  sudo dpkg -i --force-depends $(ls /usr/local/pkg/*deb)
  sudo apt --fix-broken install
  sudo sed -i '/^user_mask_fits/s|{.*}|{ /bin/true; }|' \
      /usr/lib/$(uname -m)-linux-gnu/wx/config/gtk3-unicode-3.2

  # wxWidgets#22790 patch no longer needed in wx3.2.2.1

  popd
}

set -xe
here=$(cd $(dirname $0); pwd -P)

# Load local environment if it exists i. e., this is a local build
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi
if [ -d /ci-source ]; then cd /ci-source; fi

git submodule update --init opencpn-libs

# Set up build directory and a visible link in /
builddir=build-$OCPN_TARGET
test -d $builddir || sudo mkdir $builddir  && sudo rm -rf $builddir/*
sudo chmod 777 $builddir
if [ "$PWD" != "/"  ]; then sudo ln -sf $PWD/$builddir /$builddir; fi

# Create a log file.
exec > >(tee $builddir/build.log) 2>&1;

sudo apt -qq update || apt update
sudo apt-get -qq install devscripts equivs software-properties-common

mk-build-deps --root-cmd=sudo -ir build-deps/control
rm -f *changes  *buildinfo

if [ -n "$BUILD_WX32" ]; then
  remove_wx30
  install_wx32
fi

if [ -n "$TARGET_TUPLE" ]; then
  TARGET_OPT="-DOCPN_TARGET_TUPLE=$TARGET_TUPLE";
fi

sudo apt install -q \
    python3-pip python3-setuptools python3-dev python3-wheel \
    build-essential libssl-dev libffi-dev

python3 -m pip install -q --user "urllib3<2.0.0"   # See #520
python3 -m pip install --user --upgrade -q setuptools wheel pip
python3 -m pip install --user -q cloudsmith-cli cryptography cmake

cd $builddir

cmake "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDbgInfo}" $TARGET_OPT ..
make VERBOSE=1 tarball
ldd app/*/lib/opencpn/*.so
if [ -d /ci-source ]; then
    sudo chown --reference=/ci-source -R . ../cache || :
fi
sudo chmod --reference=.. .
