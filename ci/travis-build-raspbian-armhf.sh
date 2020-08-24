#!/usr/bin/env bash


# bailout on errors and echo commands.
set -xe
sudo apt-get -qq update

DOCKER_SOCK="unix:///var/run/docker.sock"

echo "DOCKER_OPTS=\"-H tcp://127.0.0.1:2375 -H $DOCKER_SOCK -s devicemapper\"" \
    | sudo tee /etc/default/docker > /dev/null
sudo service docker restart;
sleep 5;

docker run --rm --privileged multiarch/qemu-user-static:register --reset

docker run --privileged -d -ti -e "container=docker" \
      -v ~/source_top:/source_top \
      -v $(pwd):/ci-source:rw \
      -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
      -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
      $DOCKER_IMAGE /bin/bash

sudo docker ps
DOCKER_CONTAINER_ID=$(sudo docker ps | grep raspbian | awk '{print $1}')


docker exec -ti $DOCKER_CONTAINER_ID apt-get -qq update

docker exec -ti $DOCKER_CONTAINER_ID apt-get -q -y install git cmake build-essential cmake gettext wx-common libwxgtk3.0-dev libbz2-dev libcurl4-openssl-dev libexpat1-dev libcairo2-dev libarchive-dev liblzma-dev libexif-dev lsb-release 


docker exec -ti $DOCKER_CONTAINER_ID /bin/bash -c \
    'mkdir ci-source/build; cd ci-source/build; cmake -DCMAKE_INSTALL_PREFIX=/usr ..; make; make package;'
 
echo "Stopping"
docker ps -a
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID

sudo apt -qq update
sudo apt install python3-pip python3-setuptools
pip3 install -q cloudsmith-cli
