#!/usr/bin/env bash

set -x

su -c "dnf install -q -y sudo dnf-plugins-core python3-pip openssh"
sudo dnf -y copr enable leamas/opencpn-mingw
sudo dnf -q builddep  -y build-deps/opencpn-deps.spec
rm -rf build; mkdir build; cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../mingw/fedora/toolchain.cmake \
    ..
make -j $(nproc) VERBOSE=1 tarball

# Install cloudsmith(for upload script) and cryptography (for git-push).
# https://github.com/pyca/cryptography/issues/5753 -> cryptography < 3.4
python3 -m pip install --upgrade --user -q pip setuptools
python3 -m pip install --user -q cloudsmith-cli cryptography

# python installs scripts in ~/.local/bin, teach upload.sh to use it in PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
