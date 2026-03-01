#!/usr/bin/env bash

set -e

git submodule update --init --remote

./docker-image-rm.sh

docker compose build

docker image list