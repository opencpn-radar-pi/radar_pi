#!/usr/bin/env bash

#
# Build for Raspbian in a docker container
#

set -xe

# Start up the docker image
DOCK_SOCK="unix:///var/run/docker.sock"
echo "DOCKER_OPTS=\"-H tcp://127.0.0.1:2375 -H $DOCK_SOCK -s devicemapper\"" \
    | sudo tee /etc/default/docker > /dev/null
sudo service docker restart
sleep 5;

docker run --rm --privileged multiarch/qemu-user-static:register --reset
docker run --privileged -d -ti \
    -e "container=docker" \
    -e "OCPN_TARGET=$OCPN_TARGET" \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    -v $(pwd):/ci-source:rw \
    $DOCKER_IMAGE /bin/bash
DOCKER_CONTAINER_ID=$(docker ps | awk '/raspbian/ {print $1}')


# Run build script
cat > build.sh << "EOF"
cores=$(lscpu | grep 'Core(s)' | sed 's/.*://') || cores=1
cores=$((cores + 1))
apt-get -qq update
apt-get -y install \
   git cmake build-essential cmake gettext \
   wx-common libgtk2.0-dev libwxgtk3.0-dev \
   libbz2-dev libcurl4-openssl-dev libexpat1-dev \
   libcairo2-dev libarchive-dev liblzma-dev libexif-dev lsb-release

cd /ci-source
rm -rf build; mkdir build; cd build
cmake ..
make -j$cores
make package
make repack-tarball
EOF

docker exec -ti \
    $DOCKER_CONTAINER_ID /bin/bash -xec "bash -xe /ci-source/build.sh"


# Stop and remove the container
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID
rm -f build.sh


# Install cloudsmith-cli,  required by upload.sh.
sudo apt-get -q update
sudo apt-get install python3-pip python3-setuptools
pip3 install cloudsmith-cli
