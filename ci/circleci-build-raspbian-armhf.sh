#!/usr/bin/env bash

#
# Build for Raspbian in a docker container
#
# Bugs: The buster build is real slow: https://forums.balena.io/t/85743

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
DOCKER_CONTAINER_ID=$(docker ps | awk '/balenalib/ {print $1}')


# Run build script
cat > build.sh << "EOF"
cores=$(lscpu | grep 'Core(s)' | sed 's/.*://') || cores=1
cores=$((cores + 1))
curl http://mirrordirector.raspbian.org/raspbian.public.key  | apt-key add -
curl http://archive.raspbian.org/raspbian.public.key  | apt-key add -
sudo apt -q update
sudo apt-get -y install --no-install-recommends \
   build-essential cmake file gettext git \
   libarchive-dev libbz2-dev libcairo2-dev \
   libcurl4-openssl-dev libexif-dev libexpat1-dev \
   libgtk2.0-dev libjsoncpp-dev libtinyxml-dev liblzma-dev \
   libwxgtk3.0-dev lsb-release wx-common

cd /ci-source
rm -rf build; mkdir build; cd build
cmake ..
make tarball
EOF

docker exec -ti \
    $DOCKER_CONTAINER_ID /bin/bash -xec "bash -xe /ci-source/build.sh"


# Stop and remove the container
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID
rm -f build.sh


# Install cloudsmith-cli,  required by upload.sh.
# pyenv local $(pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1)
# pip3 install cloudsmith-cli

# fix for broken cloudsmith release 0.24.0
pyenv local 3.5.2
wget https://files.pythonhosted.org/packages/59/4a/dbd1cc20156d0a9c10e2b085b8c14480166b0f765086ba2d9150495ec626/cloudsmith_cli-0.23.0-py2.py3-none-any.whl
pip3 install --user cloudsmith_cli-0.23.0-py2.py3-none-any.whl
