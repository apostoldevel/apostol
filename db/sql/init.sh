#!/bin/bash

set -e

pop_directory()
{
    popd >/dev/null
}

push_directory()
{
    local DIRECTORY="$1"

    pushd "$DIRECTORY" >/dev/null
}

create_directory()
{
    local DIRECTORY="$1"

    rm -rf "$DIRECTORY"
    mkdir "$DIRECTORY"
}

display_heading_message()
{
    echo
    echo "********************** $@ **********************"
    echo
}

display_message()
{
    echo "$@"
}

display_error()
{
    >&2 echo "$@"
}

init_db() {
  cat /tmp/postgresql.conf >> /var/lib/postgresql/$PG_MAJOR/docker/postgresql.conf
}

# Download from github.
download_from_github()
{
    local DIRECTORY=$1
    local ACCOUNT=$2
    local REPO=$3
    local BRANCH=$4
    local DIR=$5
    shift 5

    push_directory "$DIRECTORY"

    FORK="$ACCOUNT/$REPO"

    display_heading_message "Work directory: $PWD"

    if ! [ -d $DIR ]; then
      # Clone the repository locally.
      display_heading_message "Download: $FORK/$BRANCH"
      git clone --depth 1 --branch $BRANCH --single-branch "https://github.com/$FORK" $DIR
    else
      push_directory "$DIR"
      display_heading_message "Updating: $FORK/$BRANCH"
      git pull
      pop_directory
    fi

    pop_directory
}

init_db
#download_from_github /docker-entrypoint-initdb.d apostoldevel db-platform master platform
