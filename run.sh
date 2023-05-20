#!/bin/bash

docker build -t atcoder:latest .

docker run -it --rm --name atcoder \
            --memory=10g --memory-swap=12g \
            --ulimit nproc=-1 --ulimit nofile=1024 \
            --mount type=bind,src=.,dst=/work --name atcoder atcoder:latest