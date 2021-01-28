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

# Temporary fix until 3.19 is available as a pypi package
# 3.19 is needed: https://gitlab.kitware.com/cmake/cmake/-/issues/20568
url='https://dl.cloudsmith.io/public/alec-leamas/opencpn-plugins-stable/deb/debian'
wget $url/pool/${OCPN_TARGET/-*/}/main/c/cm/cmake-data_3.19.3-0.1_all.deb
wget $url/pool/${OCPN_TARGET/-*/}/main/c/cm/cmake_3.19.3-0.1_armhf.deb
sudo apt install ./cmake_3.19.3-0.1_armhf.deb ./cmake-data_3.19.3-0.1_all.deb

cd /ci-source
rm -rf build; mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so
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
# Latest pip 21.0.0 is broken:
python3 -m pip install --force-reinstall pip==20.3.4
python3 -m pip install --user cloudsmith-cli
python3 -m pip install --user cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use it in PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
