#!/usr/bin/env bash

#
# Build the flatpak artifacts.
#
set -e

MANIFEST=$(cd flatpak; ls org.opencpn.OpenCPN.Plugin*yaml)
echo "Using manifest file: $MANIFEST"
set -x

# Install the dependencies: openpcn and the flatpak SDK.
sudo dnf install -y cmake flatpak-builder flatpak gcc-c++ tar
flatpak remote-add --user --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y --or-update flathub org.opencpn.OpenCPN >/dev/null \
    || echo "(Ignored)"
flatpak install --user -y flathub org.freedesktop.Sdk//18.08  >/dev/null

# Patch the runtime version so it matches the nightly builds'
sed -i '/^runtime-version/s/:.*/: stable/' flatpak/$MANIFEST

# Configure and build the plugin tarball and metadata.
mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 flatpak

# Restore patched file so the cache checksumming is ok.
git checkout ../flatpak/$MANIFEST

# Install cloudsmith, requiered by upload script
sudo dnf install -y python3-setuptools python3-pip
python3 -m pip install --user -q --upgrade pip
python3 -m pip install --user -q cloudsmith-cli

# Required by git-push
python3 -m pip install --user -q cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use this in PATH
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
