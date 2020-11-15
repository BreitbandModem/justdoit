#!/usr/bin/env bash

docker kill habit-tracker
docker rm habit-tracker
docker build -t habit-tracker:latest -f Dockerfile .
docker run -d \
    -p 5555:5000 \
    -v $PWD:/data \
    --name habit-tracker \
    habit-tracker:latest