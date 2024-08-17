#!/bin/bash

DOCKER_IMAGE_NAME=atcoder:latest
DOCKER_CONTAINER_NAME=atcoder

if [[ "$(docker images -q $DOCKER_IMAGE_NAME 2> /dev/null)" == "" ]]
then
    docker build -t $DOCKER_IMAGE_NAME .
fi

docker run -it \
           --rm \
           --name $DOCKER_CONTAINER_NAME \
           --memory=10g \
           --memory-swap=12g \
           --ulimit nproc=-1 \
           --ulimit nofile=1024 \
           --mount type=bind,src=./,dst=/work/ \
           $DOCKER_IMAGE_NAME
