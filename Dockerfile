FROM ubuntu:20.04

ENV TZ=Asia/Tokyo
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get autoremove
RUN apt-get install -y build-essential cmake clang libssl-dev libcurl4-openssl-dev

WORKDIR /work