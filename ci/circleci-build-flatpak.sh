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
sudo systemctl kill --kill-who=all apt-daily.service


# Start up the docker instance
DOCKER_SOCK="unix:///var/run/docker.sock"
echo "DOCKER_OPTS=\"-H tcp://127.0.0.1:2375 -H $DOCKER_SOCK -s devicemapper\"" \
    | sudo tee /etc/default/docker > /dev/null

sudo systemctl restart docker.service
sudo docker pull fedora:30
docker run --privileged -d -ti -e "container=docker"  \
    -v /sys/fs/cgroup:/sys/fs/cgroup \
    -v "$(pwd):/topdir:rw" \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    fedora:30   /bin/bash
DOCKER_CONTAINER_ID=$(docker ps | awk '/fedora/ {print $1}')
docker logs $DOCKER_CONTAINER_ID


cat > build.sh  << "EOF"
cd  /topdir
su -c "dnf install -q -y sudo cmake gcc-c++ flatpak-builder flatpak make tar"
flatpak remote-add --user --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.opencpn.OpenCPN > /dev/null
flatpak install --user -y flathub org.freedesktop.Sdk//18.08  >/dev/null

sed -i '/^runtime-version/s/:.*/: stable/' \
    flatpak/org.opencpn.OpenCPN.Plugin.shipdriver.yaml

mkdir build; cd build
cmake -DOCPN_FLATPAK=ON ..
make flatpak-build
make flatpak-pkg
make repack-tarball
EOF

# Run the build in docker
docker exec -ti \
    $DOCKER_CONTAINER_ID /bin/bash -xec "bash -xe /topdir/build.sh"
docker ps -a
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID
rm -f build.sh

# Get rid of blocking daemon, install cloudsmith-cli required by upload.sh.
sudo systemctl kill --kill-who=all --signal KILL apt-daily.service
sudo rm -f /var/lib/dpkg/lock
sudo apt-get -q update
sudo apt-get install python3-pip python3-setuptools
pip3 install cloudsmith-cli
