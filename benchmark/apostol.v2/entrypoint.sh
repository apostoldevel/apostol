#!/bin/bash
set -e

echo "Starting Apostol v2 (workers=${WORKERS:-4})..."
exec /usr/sbin/apostol -w "${WORKERS:-4}"
