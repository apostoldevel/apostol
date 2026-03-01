#!/bin/bash
#
# Final benchmark script.
# Usage: docker compose --profile loadgen run --rm loadgen /bench/scripts/final-bench.sh
#
# Tests:
#   1. /ping with keep-alive ON  (t4/c100, t4/c1000)
#   2. /ping with keep-alive OFF (t4/c100, t4/c1000)
#   3. /db/ping with keep-alive OFF, direct postgres (t4/c100)
#   4. /db/ping with keep-alive OFF, via pgbouncer   (t4/c100) -- requires DB_HOST=bench-pgbouncer
#

PAUSE=5
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_DIR="/results/final_${TIMESTAMP}"
mkdir -p "$RESULT_DIR"

NGINX="http://bench-nginx:80"
APOSTOL="http://bench-apostol:8080"
PYTHON="http://bench-python:8000"
NODE="http://bench-node:3000"
GO="http://bench-go:4000"
APOSTOL_V2="http://bench-apostol-v2:8080"

run_wrk() {
  local label="$1"
  local url="$2"
  local threads="$3"
  local connections="$4"
  local ka_off="$5"
  local outfile="${RESULT_DIR}/${label}.txt"

  if [ "$ka_off" = "true" ]; then
    echo "  wrk -t${threads} -c${connections} -d10s --latency -H 'Connection: close' ${url}"
    wrk -t"${threads}" -c"${connections}" -d10s --latency -H "Connection: close" "$url" > "$outfile" 2>&1 || true
  else
    echo "  wrk -t${threads} -c${connections} -d10s --latency ${url}"
    wrk -t"${threads}" -c"${connections}" -d10s --latency "$url" > "$outfile" 2>&1 || true
  fi

  local rps=$(grep "Requests/sec" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  local lat_avg=$(grep "Latency" "$outfile" 2>/dev/null | head -1 | awk '{print $2}' || echo "N/A")
  local lat_p50=$(grep "50%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  local lat_p75=$(grep "75%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  local lat_p90=$(grep "90%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  local lat_p99=$(grep "99%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  local errors=$(grep -c "Socket errors\|Non-2xx" "$outfile" 2>/dev/null || echo "0")

  printf "    RPS: %-12s  Avg: %-10s  p50: %-10s  p75: %-10s  p90: %-10s  p99: %-10s  Errors: %s\n" \
    "${rps:-N/A}" "${lat_avg:-N/A}" "${lat_p50:-N/A}" "${lat_p75:-N/A}" "${lat_p90:-N/A}" "${lat_p99:-N/A}" "${errors}"

  echo "${label},${threads},${connections},${rps},${lat_avg},${lat_p50},${lat_p75},${lat_p90},${lat_p99},${errors}" >> "${RESULT_DIR}/summary.csv"
  sleep "$PAUSE"
}

echo "================================================================"
echo " Final Benchmark (WORKERS=4)"
echo " Output: ${RESULT_DIR}"
echo "================================================================"
echo ""

echo "label,threads,connections,rps,latency_avg,p50,p75,p90,p99,errors" > "${RESULT_DIR}/summary.csv"

# --- 1. /ping keep-alive ON ---
echo "=== /ping — keep-alive ON ==="
echo ""

for cfg in "4:100" "4:1000"; do
  IFS=':' read -r threads connections <<< "$cfg"
  echo "--- t${threads}/c${connections} ---"
  run_wrk "ping_ka-on_nginx_t${threads}_c${connections}"        "${NGINX}/api/v0/ping"         "$threads" "$connections" "false"
  run_wrk "ping_ka-on_apostol_t${threads}_c${connections}"     "${APOSTOL}/api/v1/ping"       "$threads" "$connections" "false"
  run_wrk "ping_ka-on_python_t${threads}_c${connections}"      "${PYTHON}/api/v2/ping"        "$threads" "$connections" "false"
  run_wrk "ping_ka-on_node_t${threads}_c${connections}"        "${NODE}/api/v3/ping"          "$threads" "$connections" "false"
  run_wrk "ping_ka-on_go_t${threads}_c${connections}"          "${GO}/api/v4/ping"            "$threads" "$connections" "false"
  run_wrk "ping_ka-on_apostol_v2_t${threads}_c${connections}"  "${APOSTOL_V2}/api/v5/ping"    "$threads" "$connections" "false"
  echo ""
done

# --- 2. /ping keep-alive OFF ---
echo "=== /ping — keep-alive OFF ==="
echo ""

for cfg in "4:100" "4:1000"; do
  IFS=':' read -r threads connections <<< "$cfg"
  echo "--- t${threads}/c${connections} ---"
  run_wrk "ping_ka-off_nginx_t${threads}_c${connections}"       "${NGINX}/api/v0/ping"         "$threads" "$connections" "true"
  run_wrk "ping_ka-off_apostol_t${threads}_c${connections}"    "${APOSTOL}/api/v1/ping"       "$threads" "$connections" "true"
  run_wrk "ping_ka-off_python_t${threads}_c${connections}"     "${PYTHON}/api/v2/ping"        "$threads" "$connections" "true"
  run_wrk "ping_ka-off_node_t${threads}_c${connections}"       "${NODE}/api/v3/ping"          "$threads" "$connections" "true"
  run_wrk "ping_ka-off_go_t${threads}_c${connections}"         "${GO}/api/v4/ping"            "$threads" "$connections" "true"
  run_wrk "ping_ka-off_apostol_v2_t${threads}_c${connections}" "${APOSTOL_V2}/api/v5/ping"    "$threads" "$connections" "true"
  echo ""
done

# --- 3. /db/ping keep-alive OFF, direct postgres ---
echo "=== /db/ping — keep-alive OFF, direct PostgreSQL ==="
echo ""

echo "--- t4/c100 ---"
run_wrk "dbping_direct_apostol_t4_c100"     "${APOSTOL}/api/v1/db/ping"     4 100 "true"
run_wrk "dbping_direct_python_t4_c100"      "${PYTHON}/api/v2/db/ping"      4 100 "true"
run_wrk "dbping_direct_node_t4_c100"        "${NODE}/api/v3/db/ping"        4 100 "true"
run_wrk "dbping_direct_go_t4_c100"          "${GO}/api/v4/db/ping"          4 100 "true"
run_wrk "dbping_direct_apostol_v2_t4_c100"  "${APOSTOL_V2}/api/v5/db/ping"  4 100 "true"
echo ""

# --- 4. /db/ping keep-alive OFF, via pgbouncer ---
# This section only runs if DB_HOST contains "pgbouncer"
# Otherwise prints a message to run separately with pgbouncer config
if [ "${DB_HOST}" = "bench-pgbouncer" ]; then
  echo "=== /db/ping — keep-alive OFF, via PgBouncer ==="
  echo ""

  echo "--- t4/c100 ---"
  run_wrk "dbping_pgbouncer_apostol_t4_c100"     "${APOSTOL}/api/v1/db/ping"     4 100 "true"
  run_wrk "dbping_pgbouncer_python_t4_c100"      "${PYTHON}/api/v2/db/ping"      4 100 "true"
  run_wrk "dbping_pgbouncer_node_t4_c100"        "${NODE}/api/v3/db/ping"        4 100 "true"
  run_wrk "dbping_pgbouncer_go_t4_c100"          "${GO}/api/v4/db/ping"          4 100 "true"
  run_wrk "dbping_pgbouncer_apostol_v2_t4_c100"  "${APOSTOL_V2}/api/v5/db/ping"  4 100 "true"
  echo ""
else
  echo "=== /db/ping via PgBouncer: SKIPPED ==="
  echo "  To run pgbouncer tests, restart services with:"
  echo "  WORKERS=4 DB_HOST=bench-pgbouncer DB_PORT=6432 docker compose up -d apostol python node go"
  echo "  Then re-run this script."
  echo ""
fi

echo "================================================================"
echo " Final benchmark complete. Results: ${RESULT_DIR}"
echo "================================================================"
echo ""
echo "Summary CSV: ${RESULT_DIR}/summary.csv"
