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
    -e "CLOUDSMITH_BETA_REPO=$CLOUDSMITH_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    -v $(pwd):/ci-source:rw \
    $DOCKER_IMAGE /bin/bash
DOCKER_CONTAINER_ID=$(docker ps | awk '/balenalib/ {print $1}')


# Run build script
cat > build.sh << "EOF"
curl http://mirrordirector.raspbian.org/raspbian.public.key  | apt-key add -
curl http://archive.raspbian.org/raspbian.public.key  | apt-key add -
sudo apt -q update

sudo apt install devscripts equivs
sudo mk-build-deps -ir /ci-source/build-deps/control-raspbian
sudo apt-get -q --allow-unauthenticated install -f

cd /ci-source
rm -rf build; mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 tarball
EOF

docker exec -ti \
    $DOCKER_CONTAINER_ID /bin/bash -xec "bash -xe /ci-source/build.sh"


# Stop and remove the container
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID
rm -f build.sh


# Install cloudsmith-cli,  required by upload.sh.
pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1 \
    > $HOME/.python-version
python3 -m pip install --upgrade pip
python3 -m pip install --user cloudsmith-cli
python3 -m pip install --user cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use it in PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
