#!/bin/bash

set -e

if ! [[ $PROJECT_NAME ]]; then
  PROJECT_NAME=apostol
fi

if ! [[ $LANG ]]; then
  LANG=ru_RU.UTF-8
fi

if ! [[ $PG_VERSION ]]; then
  PG_VERSION=16
fi

if ! [[ $PG_CLUSTER ]]; then
  PG_CLUSTER=main
fi

if ! [[ $PG_DATA ]]; then
  PG_DATA=/var/lib/postgresql/$PG_VERSION/$PG_CLUSTER
fi

if ! [[ $PGWEB_DATABASE_URL ]]; then
  export PGWEB_DATABASE_URL=postgres://http:http@localhost:5432/web
fi

pop_directory()
{
  popd >/dev/null
}

push_directory()
{
  local DIRECTORY="$1"
  pushd "$DIRECTORY" >/dev/null
}

display_message()
{
  echo "$@"
}

display_error()
{
  >&2 echo "$@"
}

display_help()
{
  display_message "Usage: ./run.sh [OPTION]"
  display_message "Script options:"
  display_message "  --initdb                 Initialize database."
  display_message "  --help                   Display usage, overriding script execution."
  display_message ""
}

display_configuration()
{
  display_message "--------------------------------------------------------------------"
  display_message "PROJECT_NAME    : $PROJECT_NAME"
  display_message "LANG            : $LANG"
  display_message "PG_VERSION      : $PG_VERSION"
  display_message "PG_CLUSTER      : $PG_CLUSTER"
  display_message "--------------------------------------------------------------------"
}

init_db()
{
  pg_createcluster --locale $LANG $PG_VERSION $PG_CLUSTER

  push_directory /opt/$PROJECT_NAME/docker

  cat postgresql.conf >> /etc/postgresql/$PG_VERSION/$PG_CLUSTER/postgresql.conf

  chmod 640 pg_hba.conf
  chown postgres:postgres pg_hba.conf
  cp -p pg_hba.conf /etc/postgresql/$PG_VERSION/$PG_CLUSTER/pg_hba.conf

  chmod 600 .pgpass
  chown postgres:postgres .pgpass
  cp -p .pgpass /var/lib/postgresql/.pgpass

  pop_directory

  pg_ctlcluster $PG_VERSION $PG_CLUSTER start

  push_directory /opt/$PROJECT_NAME/db/sql
  sudo -u postgres -H psql -d template1 -f make.psql
  pop_directory

  echo $(date '+%Y%m%d') > .initdb
}

display_configuration

if ! [[ -d $PG_DATA ]]; then
  init_db
else
  pg_ctlcluster $PG_VERSION $PG_CLUSTER start
fi

/usr/sbin/$PROJECT_NAME & /usr/bin/pgweb --bind=0.0.0.0 --listen=8081 --skip-open

pg_ctlcluster $PG_VERSION $PG_CLUSTER stop
