#!/usr/bin/env bash

#
# Build the flatpak artifacts.
#
set -e

MANIFEST=$(cd flatpak; ls org.opencpn.OpenCPN.Plugin*yaml)
echo "Using manifest file: $MANIFEST"
set -x

# Install the dependencies: openpcn and the flatpak SDK.
sudo dnf install -y -q cmake flatpak-builder flatpak gcc-c++ tar

# For now, horrible hack: aarch 64 builds are using the updated runtime
# 20.08 and the opencpn beta version using same runtime.
if [ "$FLATPAK_BRANCH" = 'beta' ]; then
        flatpak install --user -y flathub org.freedesktop.Sdk//20.08 >/dev/null
        flatpak remote-add --user --if-not-exists flathub-beta \
            https://flathub.org/beta-repo/flathub-beta.flatpakrepo
        flatpak install --user -y --or-update flathub-beta \
            org.opencpn.OpenCPN >/dev/null
        sed -i '/sdk:/s/18.08/20.08/'  flatpak/org.opencpn.*.yaml
else
        flatpak install --user -y flathub org.freedesktop.Sdk//18.08 >/dev/null
        flatpak remote-add --user --if-not-exists flathub \
            https://flathub.org/repo/flathub.flatpakrepo
        flatpak install --user -y --or-update flathub \
            org.opencpn.OpenCPN >/dev/null
        FLATPAK_BRANCH='stable'
fi

# Patch the runtime version so it matches the nightly builds
# or beta as appropriate.
sed -i "/^runtime-version/s/:.*/: $FLATPAK_BRANCH/" flatpak/$MANIFEST

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
