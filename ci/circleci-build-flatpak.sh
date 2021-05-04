#!/usr/bin/env bash

#
# Build the flatpak artifacts.
#
set -e

MANIFEST=$(cd flatpak; ls org.opencpn.OpenCPN.Plugin*yaml)
echo "Using manifest file: $MANIFEST"
set -x

# On old systems: give the apt update daemons a chance to leave the scene
# while we build.
if systemctl status apt-daily-upgrade.service &> /dev/null; then
    sudo systemctl stop apt-daily.service apt-daily.timer
    sudo systemctl kill --kill-who=all \
        apt-daily.service apt-daily-upgrade.service
    sudo systemctl mask apt-daily.service apt-daily-upgrade.service
    sudo systemctl daemon-reload
fi

# Install the flatpak PPA so we can access a usable flatpak version
# (not available on Ubuntu Xenial 16.04)
if [ -n "$USE_CUSTOM_PPA" ]; then
    sudo add-apt-repository -y ppa:alexlarsson/flatpak
    wget -q -O - https://dl.google.com/linux/linux_signing_key.pub \
        | sudo apt-key add -
fi
sudo apt update

sudo apt install flatpak flatpak-builder
flatpak remote-add --user --if-not-exists \
    flathub https://dl.flathub.org/repo/flathub.flatpakrepo


# For now, horrible hack: aarch 64 builds are using the updated runtime
# 20.08 and the opencpn beta version using old 18.08 runtime.
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

# The flatpak checksumming needs python3:
pyenv local $(pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1)
cp .python-version $HOME

# Install a recent cmake, ubuntu 16.04 is too old.
export PATH=$HOME/.local/bin:$PATH
python -m pip install --user cmake

# Configure and build the plugin tarball and metadata.
mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 flatpak

# Restore patched file so the cache checksumming is ok.
git checkout ../flatpak/$MANIFEST

# Wait for apt-daily to complete. apt-daily should not restart, it's masked.
# https://unix.stackexchange.com/questions/315502
echo -n "Waiting for apt_daily lock..."
sudo flock /var/lib/apt/daily_lock echo done

# Install cloudsmith, required by upload script
python3 -m pip install --user --upgrade pip
python3 -m pip install --user cloudsmith-cli

# Required by git-push
python3 -m pip install --user cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use this in PATH
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
