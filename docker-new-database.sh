#!/usr/bin/env bash
set -euo pipefail

echo
read -r -p "Recreate the database? (y/N): " ans
if [[ "${ans}" != "y" && "${ans}" != "Y" ]]; then
  echo "Cancel."
  exit 0
fi

./docker-down.sh

docker volume rm apostol_postgresql

docker compose up -d --force-recreate postgres
docker compose logs -fn 500
