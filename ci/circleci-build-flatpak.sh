#!/usr/bin/env bash

#
# Build the flatpak artifacts.
#
set -e

MANIFEST=$(cd flatpak; ls org.opencpn.OpenCPN.Plugin*yaml)
echo "Using manifest file: $MANIFEST"
set -x

sudo apt update


# Install flatpak and flatpak-builder
sudo apt install flatpak flatpak-builder
flatpak remote-add --user --if-not-exists \
    flathub https://dl.flathub.org/repo/flathub.flatpakrepo

# For now, horrible hack: aarch 64 builds are using the updated runtime
# 20.08 and the opencpn beta version using old 18.08 runtime.

commit_1808=959f5fd700f72e63182eabb9821b6aa52fb12189eddf72ccf99889977b389447
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
        flatpak update --user --commit $commit_1808 org.opencpn.OpenCPN
        FLATPAK_BRANCH='stable'
fi

# Patch the runtime version so it matches the nightly builds
# or beta as appropriate.
sed -i "/^runtime-version/s/:.*/: $FLATPAK_BRANCH/" flatpak/$MANIFEST

# The flatpak checksumming needs python3:
pyenv local $(pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1)
cp .python-version $HOME

# Configure and build the plugin tarball and metadata.
mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 flatpak

# Restore patched file so the cache checksumming is ok.
git checkout ../flatpak/$MANIFEST

# Install cloudsmith and cryptography, required by upload script and git-push
python3 -m pip install -q --user --upgrade pip
python3 -m pip install -q --user cloudsmith-cli cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use this in PATH
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
