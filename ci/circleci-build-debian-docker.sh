#!/usr/bin/env bash
#
# Build for Debian in a docker container
#
# Copyright (c) 2021 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set -xe

# Create build script
#
if [ -n "$TRAVIS_BUILD_DIR" ]; then
    ci_source="$TRAVIS_BUILD_DIR"
elif [ -d ~/project ]; then
    ci_source=~/project
else
    ci_source="$(pwd)"
fi

cd $ci_source
git submodule update --init opencpn-libs

cat > $ci_source/build.sh << "EOF"
function remove_wx30() {
  apt remove -y \
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
  test -d /usr/local/pkg || mkdir /usr/local/pkg
  chmod a+w /usr/local/pkg
  repo="https://dl.cloudsmith.io/public/alec-leamas/wxwidgets-32"
  head="deb/debian/pool/bullseye/main"
  vers="3.2.2+dfsg-1~bpo11+1"
  pushd /usr/local/pkg
  wget -q $repo/$head/w/wx/wx-common_${vers}/wx-common_${vers}_arm64.deb
  wget -q $repo/$head/w/wx/wx3.2-i18n_${vers}/wx3.2-i18n_${vers}_all.deb
  wget -q $repo/$head/w/wx/wx3.2-headers_${vers}/wx3.2-headers_${vers}_all.deb
  wget -q $repo/$head/l/li/libwxgtk-webview3.2-dev_${vers}/libwxgtk-webview3.2-dev_${vers}_arm64.deb
  wget -q $repo/$head/l/li/libwxgtk-webview3.2-1_${vers}/libwxgtk-webview3.2-1_${vers}_arm64.deb
  wget -q $repo/$head/l/li/libwxgtk-media3.2-dev_${vers}/libwxgtk-media3.2-dev_${vers}_arm64.deb
  wget -q $repo/$head/l/li/libwxgtk3.2-dev_${vers}/libwxgtk3.2-dev_${vers}_arm64.deb
  wget -q $repo/$head/l/li/libwxgtk3.2-1_${vers}/libwxgtk3.2-1_${vers}_arm64.deb
  wget -q $repo/$head/l/li/libwxgtk-gl3.2-1_${vers}/libwxgtk-gl3.2-1_${vers}_arm64.deb
  wget -q $repo/$head/l/li/libwxbase3.2-1_${vers}/libwxbase3.2-1_${vers}_arm64.deb
  wget -q $repo/$head/l/li/libwxgtk-media3.2-1_${vers}/libwxgtk-media3.2-1_${vers}_arm64.deb

  dpkg -i --force-depends $(ls /usr/local/pkg/*deb)
  sed -i '/^user_mask_fits/s|{.*}|{ /bin/true; }|' \
      /usr/lib/*-linux-gnu/wx/config/gtk3-unicode-3.2

  # wxWidgets#22790 patch no longer needed in wx3.2.2.1

  popd
}

set -x

apt -y update
apt -y install acl devscripts equivs wget git lsb-release

mk-build-deps /ci-source/build-deps/control
apt install -q -y ./opencpn-build-deps*deb
apt-get -q --allow-unauthenticated install -f

debian_rel=$(lsb_release -sc)
if [ "$debian_rel" = bullseye ]; then
    echo "deb http://deb.debian.org/debian bullseye-backports main" \
      >> /etc/apt/sources.list
    apt update
    apt install -y cmake/bullseye-backports
else
    apt-get install -y cmake
fi

if [ -n "@BUILD_WX32@" ]; then
  remove_wx30
  install_wx32
fi

cd /ci-source
getfacl -R /ci-source > /ci-source.permissions

chown root:root /ci-source
git config --global --add safe.directory /ci-source

rm -rf build-debian; mkdir build-debian; cd build-debian
cmake "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}" \
   -DOCPN_TARGET_TUPLE="@TARGET_TUPLE@" \
    ..

git config --global --add safe.directory /ci-source

make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so

cd /
setfacl --restore=/ci-source.permissions
EOF

sed -i "s/@TARGET_TUPLE@/$TARGET_TUPLE/" $ci_source/build.sh
sed -i "s/@BUILD_WX32@/$BUILD_WX32/" $ci_source/build.sh

# Run script in docker image
#
docker run \
    -e "CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM" \
    -e "TRAVIS_BUILD_NUMBER" \
    -v "$ci_source:/ci-source:rw" \
    debian:$OCPN_TARGET /bin/bash -xe /ci-source/build.sh
rm -f $ci_source/build.sh


# Install cloudsmith-cli (for upload) and cryptography (for git-push).
#
if pyenv versions &>/dev/null;  then
    pyenv versions | tr -d '*' | awk '{print $1}' | tail -1 \
        > $HOME/.python-version
fi
python3 -m pip install -q --user "urllib3<2.0.0"   # See #520
python3 -m pip install -q --user cloudsmith-cli cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use in it's PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
