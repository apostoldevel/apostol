#!/bin/bash

set -euo pipefail

# Установка значений переменных по умолчанию
PROJECT_NAME="${PROJECT_NAME:-apostol}"

set -o allexport
source /opt/$PROJECT_NAME/.env
set +o allexport

PGHOST="${PGHOST:-postgres}"
PGPORT="${PGPORT:-5432}"
PGUSER="${PGUSER:-http}"
PGDATABASE="${PGDATABASE:-web}"

PG_PARAMS=(-d "$PGDATABASE" -U "$PGUSER")

if [ -n "$PGHOST" ]; then
  PG_PARAMS+=(-h "$PGHOST")
fi

if [ -n "$PGPORT" ]; then
  PG_PARAMS+=(-p "$PGPORT")
fi

# Удаление логов, если каталог существует
if [[ -d /var/log/$PROJECT_NAME ]]; then
  find /var/log/"$PROJECT_NAME" -name '*.log' -delete
fi

pop_directory() {
  popd >/dev/null
}

push_directory() {
  local DIRECTORY="$1"
  pushd "$DIRECTORY" >/dev/null
}

display_message() {
  echo "$@"
}

display_error() {
  >&2 echo "$@"
}

display_configuration() {
  display_message "--------------------------------------------------------------------"
  display_message "PROJECT_NAME    : $PROJECT_NAME"
  display_message "WORKER_PROCESSES: $WORKER_PROCESSES"
  display_message "PGHOST          : $PGHOST"
  display_message "PGPORT          : $PGPORT"
  display_message "PGDATABASE      : $PGDATABASE"
  display_message "PGUSER          : $PGUSER"
  display_message "--------------------------------------------------------------------"
}

set_env() {
  push_directory /opt/"$PROJECT_NAME"
  envsubst < conf/sites/default.json > /etc/"$PROJECT_NAME"/sites/default.json
  envsubst < conf/default.json > /etc/"$PROJECT_NAME"/conf/"$PROJECT_NAME".json
  pop_directory
}

init_app() {
  mkdir -p /etc/"$PROJECT_NAME"/conf /etc/"$PROJECT_NAME"/oauth2 /etc/"$PROJECT_NAME"/sites
  push_directory /opt/"$PROJECT_NAME"
  cp -p conf/*.json /etc/"$PROJECT_NAME"/conf
  pop_directory
}

display_configuration

if [[ ! -f /etc/$PROJECT_NAME/conf/$PROJECT_NAME.conf ]]; then
  init_app
fi

set_env

display_message "Waiting for the database to be ready..."

retries=10
until pg_isready --timeout=1 "${PG_PARAMS[@]}" >/dev/null 2>&1; do
  sleep 1
  ((retries=retries-1))
  if [ $retries -eq 0 ]; then
    display_error "Can't connect to database $PGDATABASE on $PGHOST:$PGPORT as user $PGUSER."
    exit 1
  fi
done

/usr/sbin/"$PROJECT_NAME" -w "$WORKER_PROCESSES"
