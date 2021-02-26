#!/usr/bin/env bash

#
# Build the flatpak artifacts.
#
set -e

MANIFEST=$(cd flatpak; ls org.opencpn.OpenCPN.Plugin*yaml)
echo "Using manifest file: $MANIFEST"
set -x

# Install the flatpak PPA so we can access a usable flatpak version
# (not available on Ubuntu Xenial 16.04, the VM this runs on)
sudo add-apt-repository -y ppa:alexlarsson/flatpak
wget -q -O - https://dl.google.com/linux/linux_signing_key.pub \
    | sudo apt-key add -
sudo apt update

# Install the dependencies: openpcn and the flatpak SDK.
sudo apt install build-essential flatpak-builder flatpak tar
flatpak remote-add --user --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.opencpn.OpenCPN > /dev/null
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
sudo apt install python3-setuptools python3-pip
python3 -m pip install --user --upgrade pip
python3 -m pip install --user cloudsmith-cli

# Required by git-push
python3 -m pip install --user cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use this in PATH
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
