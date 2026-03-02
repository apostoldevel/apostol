#!/usr/bin/env bash

set -e

docker compose up -d --force-recreate
docker compose logs -n 500

