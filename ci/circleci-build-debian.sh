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

  # Necessary in my own dev env, not on CircleCI:
  sudo apt remove libwxbase3.2-0 \
      libwxgtk3.2-0 \
      libwxgtk-webview3.2-0 || :

}

# Install generated packages
function install_wx32() {
  test -d /usr/local/pkg || sudo mkdir /usr/local/pkg
  sudo chmod a+w /usr/local/pkg
  repo="https://ppa.launchpadcontent.net/opencpn/opencpn/"
  head="ubuntu/pool/main/w/wxwidgets3.2"
  vers="3.2.2+dfsg-1~bpo22.04.1"
  pushd /usr/local/pkg
  wget -v -r -l 1 -np -nd -A _all.deb,_amd64.deb "$repo/$head/"
  #wget -q $repo/$head/wx-common_${vers}_amd64.deb
  #wget -q $repo/$head/wx3.2-i18n_${vers}_all.deb
  #wget -q $repo/$head/wx3.2-headers_${vers}_all.deb
  #wget -q $repo/$head/libwxgtk3.2-dev_${vers}_amd64.deb
  #wget -q $repo/$head/libwxgtk3.2-1_${vers}_amd64.deb
  #wget -q $repo/$head/libwxgtk-gl3.2-1_${vers}_amd64.deb
  #wget -q $repo/$head/libwxbase3.2-1_${vers}_amd64.deb
  sudo dpkg -i --force-depends $(ls /usr/local/pkg/*deb)
  sudo apt --fix-broken install
  sudo sed -i '/^user_mask_fits/s|{.*}|{ /bin/true; }|' \
      /usr/lib/$(uname -m)-linux-gnu/wx/config/gtk3-unicode-3.2
  # See wxWidgets#22790. To be removed after wxw 3.2.3
  #cd /usr/include/wx-3.2/wx/
  #sudo patch -p1 \
    #< $here/../build-deps/0001-matrix.h-Patch-attributes-handling-wxwidgets-22790.patch
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
#if [ "$PWD" != "/"  ]; then sudo ln -sf $PWD/$builddir /$builddir; fi

# Create a log file.
exec > >(tee $builddir/build.log) 2>&1;

sudo apt -qq update || apt update
sudo apt-get -qq install devscripts equivs software-properties-common

mk-build-deps --root-cmd=sudo -ir build-deps/control
rm -f *changes  *buildinfo

if [ -n "$BUILD_WX32" ]; then
  remove_wx30;
  install_wx32;
  OCPN_WX_ABI_OPT="-DOCPN_WX_ABI=wx32"
fi

if [ -n "$TARGET_TUPLE" ]; then
  TARGET_OPT="-DOCPN_TARGET_TUPLE=$TARGET_TUPLE";
fi

sudo apt install -q \
    python3-pip python3-setuptools python3-dev python3-wheel \
    build-essential libssl-dev libffi-dev

[ -z "${SKIP_PIP:-}" ] && python3 -m pip install --user --upgrade -q setuptools wheel pip
[ -z "${SKIP_PIP:-}" ] && python3 -m pip install --user -q cloudsmith-cli cryptography cmake

cd $builddir

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo $OCPN_WX_ABI_OPT $TARGET_OPT ..
make VERBOSE=1 tarball
ldd app/*/lib/opencpn/*.so
if [ -d /ci-source ]; then
    sudo chown --reference=/ci-source -R . ../cache || :
fi
sudo chmod --reference=.. .
