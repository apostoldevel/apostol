#!/bin/bash
#
# Quick 10-second debug test across all endpoints.
# Usage: docker compose --profile loadgen run --rm loadgen /bench/scripts/debug-quick.sh
#

set -e

DURATION=10s
THREADS=2
CONNECTIONS=100

NGINX="http://bench-nginx:80"
APOSTOL="http://bench-apostol:8080"
PYTHON="http://bench-python:8000"
NODE="http://bench-node:3000"
GO="http://bench-go:4000"

ENDPOINTS=(
  "ping"
  "time"
  "db/ping"
  "db/time"
)

SERVICES=(
  "v0:nginx:${NGINX}"
  "v1:apostol:${APOSTOL}"
  "v2:python:${PYTHON}"
  "v3:node:${NODE}"
  "v4:go:${GO}"
)

echo "========================================"
echo " Quick Debug Benchmark (${DURATION})"
echo "========================================"
echo ""

# Verify all services are reachable
echo "--- Connectivity check ---"
for svc in "${SERVICES[@]}"; do
  IFS=':' read -r version name base_url <<< "$svc"
  for ep in "${ENDPOINTS[@]}"; do
    url="${base_url}/api/${version}/${ep}"
    status=$(curl -s -o /dev/null -w '%{http_code}' "$url" 2>/dev/null || echo "FAIL")
    printf "  %-10s /api/%s/%-10s -> %s\n" "$name" "$version" "$ep" "$status"
  done
done
echo ""

# Run wrk tests
echo "--- wrk tests (${THREADS}t/${CONNECTIONS}c/${DURATION}) ---"
echo ""

for svc in "${SERVICES[@]}"; do
  IFS=':' read -r version name base_url <<< "$svc"
  for ep in "${ENDPOINTS[@]}"; do
    url="${base_url}/api/${version}/${ep}"
    printf "%-10s /api/%s/%s\n" "$name" "$version" "$ep"
    wrk -t${THREADS} -c${CONNECTIONS} -d${DURATION} "$url" 2>&1 | grep -E "Requests/sec|Latency|Transfer"
    echo ""
  done
done

echo "========================================"
echo " Debug test complete"
echo "========================================"
