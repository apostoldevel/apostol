#!/bin/bash
#
# Ping benchmark script.
# Usage: docker compose --profile loadgen run --rm loadgen /bench/scripts/bench_ping.sh [OPTIONS]
#
# Tests:
#   1. /api/vN/ping  — keep-alive ON  (t4/c100, t4/c1000)
#   2. /api/vN/ping  — keep-alive OFF (t4/c100, t4/c1000)
#   3. /api/vN/db/ping — keep-alive ON  (t4/c100)
#   4. /api/vN/db/ping — keep-alive OFF (t4/c100)
#
# Options:
#   --duration=<sec>     Test duration in seconds (default: 10)
#   --pause=<sec>        Pause between tests (default: 5)
#   --workers=<n>        Worker count label (default: $WORKERS or 4)
#

set -o pipefail

# --- Defaults ---
DURATION=10
PAUSE=5
WORKERS="${WORKERS:-4}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT="/results"

# --- Parse args ---
for arg in "$@"; do
  case $arg in
    --duration=*)  DURATION="${arg#*=}" ;;
    --pause=*)     PAUSE="${arg#*=}" ;;
    --workers=*)   WORKERS="${arg#*=}" ;;
    --output=*)    OUTPUT="${arg#*=}" ;;
    *)             echo "Unknown option: $arg"; exit 1 ;;
  esac
done

RESULT_DIR="${OUTPUT}/ping_${TIMESTAMP}_w${WORKERS}"
mkdir -p "$RESULT_DIR"
SUMMARY="${RESULT_DIR}/summary.csv"

# --- Targets ---
declare -A TARGETS=(
  ["v0-nginx"]="http://bench-nginx:80"
  ["v1-apostol"]="http://bench-apostol:8080"
  ["v2-python"]="http://bench-python:8000"
  ["v3-node"]="http://bench-node:3000"
  ["v4-go"]="http://bench-go:4000"
  ["v5-apostol_v2"]="http://bench-apostol-v2:8080"
)

# Services with DB access (for /db/ping)
DB_TARGETS=("v1-apostol" "v2-python" "v3-node" "v4-go" "v5-apostol_v2")

# Display order
ALL_TARGETS=("v0-nginx" "v1-apostol" "v2-python" "v3-node" "v4-go" "v5-apostol_v2")

# --- Functions ---

run_wrk() {
  local label="$1" url="$2" threads="$3" connections="$4" ka_off="$5"
  local outfile="${RESULT_DIR}/${label}.txt"

  if [ "$ka_off" = "true" ]; then
    echo "  wrk -t${threads} -c${connections} -d${DURATION}s --latency -H 'Connection: close' ${url}"
    wrk -t"${threads}" -c"${connections}" -d"${DURATION}s" --latency -H "Connection: close" "$url" > "$outfile" 2>&1 || true
  else
    echo "  wrk -t${threads} -c${connections} -d${DURATION}s --latency ${url}"
    wrk -t"${threads}" -c"${connections}" -d"${DURATION}s" --latency "$url" > "$outfile" 2>&1 || true
  fi

  local rps lat_avg lat_p50 lat_p75 lat_p90 lat_p99 errors
  rps=$(grep "Requests/sec" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  lat_avg=$(grep "Latency" "$outfile" 2>/dev/null | head -1 | awk '{print $2}' || echo "N/A")
  lat_p50=$(grep "50%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  lat_p75=$(grep "75%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  lat_p90=$(grep "90%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  lat_p99=$(grep "99%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  errors=$(grep -c "Socket errors\|Non-2xx" "$outfile" 2>/dev/null || echo "0")

  printf "    RPS: %-12s  Avg: %-10s  p50: %-10s  p99: %-10s  Err: %s\n" \
    "${rps:-N/A}" "${lat_avg:-N/A}" "${lat_p50:-N/A}" "${lat_p99:-N/A}" "${errors}"

  echo "${label},${threads},${connections},${rps},${lat_avg},${lat_p50},${lat_p75},${lat_p90},${lat_p99},${errors}" >> "$SUMMARY"
  sleep "$PAUSE"
}

# --- Main ---

echo "================================================================"
echo " Ping Benchmark (WORKERS=${WORKERS})"
echo " Duration: ${DURATION}s | Pause: ${PAUSE}s"
echo " Output:   ${RESULT_DIR}"
echo "================================================================"
echo ""

# --- Connectivity check ---
echo "Checking connectivity..."
AVAILABLE=()
for target_key in "${ALL_TARGETS[@]}"; do
  base_url="${TARGETS[$target_key]}"
  version="${target_key%%-*}"
  if curl -sf --max-time 3 "${base_url}/api/${version}/ping" > /dev/null 2>&1; then
    echo "  ${target_key}: OK"
    AVAILABLE+=("$target_key")
  else
    echo "  ${target_key}: UNAVAILABLE (skipping)"
  fi
done
echo ""

# --- CSV header ---
echo "label,threads,connections,rps,latency_avg,p50,p75,p90,p99,errors" > "$SUMMARY"

# === 1. /ping — keep-alive ON ===
echo "=== /ping — keep-alive ON ==="
echo ""

for cfg in "4:100" "4:1000"; do
  IFS=':' read -r threads connections <<< "$cfg"
  echo "--- t${threads}/c${connections} ---"
  for target_key in "${AVAILABLE[@]}"; do
    base_url="${TARGETS[$target_key]}"
    version="${target_key%%-*}"
    run_wrk "ping_ka-on_${target_key}_t${threads}_c${connections}" \
            "${base_url}/api/${version}/ping" "$threads" "$connections" "false"
  done
  echo ""
done

# === 2. /ping — keep-alive OFF ===
echo "=== /ping — keep-alive OFF ==="
echo ""

for cfg in "4:100" "4:1000"; do
  IFS=':' read -r threads connections <<< "$cfg"
  echo "--- t${threads}/c${connections} ---"
  for target_key in "${AVAILABLE[@]}"; do
    base_url="${TARGETS[$target_key]}"
    version="${target_key%%-*}"
    run_wrk "ping_ka-off_${target_key}_t${threads}_c${connections}" \
            "${base_url}/api/${version}/ping" "$threads" "$connections" "true"
  done
  echo ""
done

# === 3. /db/ping — keep-alive ON (t4/c100 only) ===
echo "=== /db/ping — keep-alive ON (t4/c100) ==="
echo ""

for target_key in "${DB_TARGETS[@]}"; do
  # Check if available
  found=false
  for t in "${AVAILABLE[@]}"; do [ "$t" = "$target_key" ] && found=true; done
  [ "$found" = "false" ] && continue

  base_url="${TARGETS[$target_key]}"
  version="${target_key%%-*}"
  run_wrk "dbping_ka-on_${target_key}_t4_c100" \
          "${base_url}/api/${version}/db/ping" 4 100 "false"
done
echo ""

# === 4. /db/ping — keep-alive OFF (t4/c100 only) ===
echo "=== /db/ping — keep-alive OFF (t4/c100) ==="
echo ""

for target_key in "${DB_TARGETS[@]}"; do
  # Check if available
  found=false
  for t in "${AVAILABLE[@]}"; do [ "$t" = "$target_key" ] && found=true; done
  [ "$found" = "false" ] && continue

  base_url="${TARGETS[$target_key]}"
  version="${target_key%%-*}"
  run_wrk "dbping_ka-off_${target_key}_t4_c100" \
          "${base_url}/api/${version}/db/ping" 4 100 "true"
done
echo ""

# === Summary ===
echo "================================================================"
echo " Benchmark complete. Results: ${RESULT_DIR}"
echo "================================================================"
echo ""
echo "Summary CSV: ${SUMMARY}"
echo ""

# --- Print summary table ---
echo "=== SUMMARY TABLE ==="
echo ""
printf "%-50s %4s %6s %12s %10s %10s %10s %4s\n" \
  "Label" "T" "C" "RPS" "Avg" "p50" "p99" "Err"
printf '%.0s-' {1..108}
echo ""

tail -n +2 "$SUMMARY" | while IFS=',' read -r label threads connections rps lat_avg p50 p75 p90 p99 errors; do
  printf "%-50s %4s %6s %12s %10s %10s %10s %4s\n" \
    "$label" "$threads" "$connections" "$rps" "$lat_avg" "$p50" "$p99" "$errors"
done
