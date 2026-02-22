#!/bin/bash
set -e

# Substitute environment variables in config
envsubst < /etc/apostol/apostol.conf.tpl > /etc/apostol/apostol.conf

# Wait for PostgreSQL to be ready
until pg_isready -h "$DB_HOST" -p "$DB_PORT" -U bench -q; do
  echo "Waiting for PostgreSQL at $DB_HOST:$DB_PORT..."
  sleep 1
done

echo "PostgreSQL is ready. Starting Apostol..."
exec /usr/sbin/apostol
