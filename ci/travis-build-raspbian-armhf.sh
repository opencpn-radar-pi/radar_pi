#!/usr/bin/env bash

#
# Build for raspbian armhf in a qemu docker container
#

set -xe

# Start up the docker instance
DOCKER_SOCK="unix:///var/run/docker.sock"
echo "DOCKER_OPTS=\"-H tcp://127.0.0.1:2375 -H $DOCKER_SOCK -s devicemapper\"" \
    | sudo tee /etc/default/docker > /dev/null
sudo systemctl restart docker.service

docker run --rm --privileged multiarch/qemu-user-static:register --reset
docker run --privileged -d -ti \
      -v $(pwd):/ci-source:rw \
      -e "container=docker" \
      -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
      -e "CLOUDSMITH_BETA_REPO=$CLOUDSMITH_BETA_REPO" \
      -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
      -e "TRAVIS_BUILD_NUMBER=$TRAVIS_BUILD_NUMBER" \
      $DOCKER_IMAGE /bin/bash
sudo docker ps
DOCKER_CONTAINER_ID=$(sudo docker ps | awk '/raspbian/ {print $1}')

cat << "EOF" >build.sh
apt-get -qq update
apt-get -q -y install \
    git cmake build-essential gettext wx-common \
    libwxgtk3.0-dev libbz2-dev libcurl4-openssl-dev \
    libexpat1-dev libcairo2-dev libarchive-dev \
    liblzma-dev libexif-dev lsb-release

mkdir ci-source/build
cd ci-source/build
cmake ..
make tarball
EOF

# Run the build script
docker exec -ti \
    $DOCKER_CONTAINER_ID /bin/bash -xec "bash -xe /ci-source/build.sh"

# Stop docker container and get rid of it
docker ps -a
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID
rm -f build.sh

# Install cloudsmith-cli, required by upload.sh
sudo apt-get -qq update
sudo apt -q install python3-pip python3-setuptools

pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1 \
    > $HOME/.python-version
python3 -m pip install --upgrade pip
python3 -m pip install --user -q cloudsmith-cli
python3 -m pip install --user -q cryptography

# python install scripts in ~/.local/bin:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.bashrc
