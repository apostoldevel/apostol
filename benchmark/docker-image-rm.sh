#!/usr/bin/env bash

set -e

PROJECT_NAME=benchmark

# Will get a list of all images, filter them by repository name and delete them
docker image list --format '{{.Repository}}:{{.Tag}}' | grep "^$PROJECT_NAME-" | while read -r image; do
  echo "Deleting an image: $image"
  docker image rm "$image"
done

docker image list
