#!/usr/bin/env bash

#
# Build the flatpak artifacts. Uses docker to run Fedora on
# in full-fledged VM; the actual build is done in the Fedora
# container. 
#
# flatpak-builder can be run in a docker image. However, this
# must then be run in privileged mode, which means we need 
# a full-fledged VM to run it.
#

set -xe

# Give the apt update daemons a chance to leave the scene.
sudo systemctl stop apt-daily.service apt-daily.timer
sudo systemctl kill --kill-who=all apt-daily.service apt-daily-upgrade.service
sudo systemctl mask apt-daily.service apt-daily-upgrade.service
sudo systemctl daemon-reload

sudo add-apt-repository -y ppa:alexlarsson/flatpak
wget -q -O - https://dl.google.com/linux/linux_signing_key.pub \
    | sudo apt-key add -
sudo apt update

sudo apt install build-essential flatpak-builder flatpak tar
flatpak remote-add --user --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.opencpn.OpenCPN > /dev/null
flatpak install --user -y flathub org.freedesktop.Sdk//18.08  >/dev/null
cp flatpak/org.opencpn.OpenCPN.Plugin.shipdriver.yaml flatpak.yaml.bak
sed -i '/^runtime-version/s/:.*/: stable/' \
    flatpak/org.opencpn.OpenCPN.Plugin.shipdriver.yaml

mkdir build; cd build
cmake  ..
make flatpak

# Restore file so the cache checksumming is ok.
cp ../flatpak.yaml.bak org.opencpn.OpenCPN.Plugin.shipdriver.yaml

# Wait for apt-daily to complete, install cloudsmith-cli required by upload.sh.
# apt-daily should not restart, it's masked.
# https://unix.stackexchange.com/questions/315502
echo -n "Waiting for apt_daily lock..."
sudo flock /var/lib/apt/daily_lock echo done

pyenv local $(pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1)
pip3 install cloudsmith-cli
