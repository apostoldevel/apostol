#!/usr/bin/env bash

set -e

./docker-image-rm.sh

docker compose build

docker image list