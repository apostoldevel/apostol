#!/bin/bash
#
# Main benchmark orchestration script.
# Usage: docker compose --profile loadgen run --rm loadgen /bench/scripts/bench.sh [OPTIONS]
#
# Options:
#   --duration=<sec>     Test duration in seconds (default: 10)
#   --workers=<n>        Number of workers (informational, for labeling)
#   --output=<dir>       Output directory (default: /results)
#   --quick              Quick mode: fewer thread/connection configs
#

# --- Defaults ---
DURATION=10
PAUSE=5
OUTPUT="/results"
QUICK=false
WORKERS="${WORKERS:-4}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# --- Parse args ---
for arg in "$@"; do
  case $arg in
    --duration=*)  DURATION="${arg#*=}" ;;
    --workers=*)   WORKERS="${arg#*=}" ;;
    --output=*)    OUTPUT="${arg#*=}" ;;
    --quick)       QUICK=true ;;
    *)             echo "Unknown option: $arg"; exit 1 ;;
  esac
done

RESULT_DIR="${OUTPUT}/${TIMESTAMP}_w${WORKERS}"
mkdir -p "$RESULT_DIR"

# --- Targets ---
NGINX="http://bench-nginx:80"
APOSTOL="http://bench-apostol:8080"
PYTHON="http://bench-python:8000"
NODE="http://bench-node:3000"
GO="http://bench-go:4000"
APOSTOL_V2="http://bench-apostol-v2:8080"

ENDPOINTS=("ping" "time" "db/ping" "db/time")

# Thread/connection configurations
if [ "$QUICK" = true ]; then
  CONFIGS=("4:100" "4:1000")
else
  CONFIGS=("4:100" "4:1000" "4:5000" "4:10000")
fi

# --- Functions ---

run_wrk() {
  local label="$1"
  local url="$2"
  local threads="$3"
  local connections="$4"
  local close_conn="${5:-false}"
  local outfile="${RESULT_DIR}/${label}_t${threads}_c${connections}.txt"

  if [ "$close_conn" = "true" ]; then
    echo "  wrk -t${threads} -c${connections} -d${DURATION}s -H 'Connection: close' ${url}"
    wrk -t"${threads}" -c"${connections}" -d"${DURATION}s" -H "Connection: close" "$url" > "$outfile" 2>&1 || true
  else
    echo "  wrk -t${threads} -c${connections} -d${DURATION}s ${url}"
    wrk -t"${threads}" -c"${connections}" -d"${DURATION}s" "$url" > "$outfile" 2>&1 || true
  fi

  # Extract key metrics (grep || true to avoid exit on no match)
  local rps=$(grep "Requests/sec" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  local lat_avg=$(grep "Latency" "$outfile" 2>/dev/null | head -1 | awk '{print $2}' || echo "N/A")
  local lat_p99=$(grep "99%" "$outfile" 2>/dev/null | awk '{print $2}' || echo "N/A")
  printf "    RPS: %-12s  Avg Latency: %-10s  p99: %s\n" "${rps:-N/A}" "${lat_avg:-N/A}" "${lat_p99:-N/A}"
  sleep "$PAUSE"
}

# --- Main ---

echo "================================================================"
echo " REST API Benchmark"
echo " Duration: ${DURATION}s | Workers: ${WORKERS}"
echo " Output:   ${RESULT_DIR}"
echo "================================================================"
echo ""

# --- 1. Direct access tests ---

declare -A TARGETS=(
  ["v0-nginx"]="${NGINX}"
  ["v1-apostol"]="${APOSTOL}"
  ["v2-python"]="${PYTHON}"
  ["v3-node"]="${NODE}"
  ["v4-go"]="${GO}"
  ["v5-apostol_v2"]="${APOSTOL_V2}"
)

for target_key in v0-nginx v1-apostol v2-python v3-node v4-go v5-apostol_v2; do
  base_url="${TARGETS[$target_key]}"
  version="${target_key%%-*}"

  echo "--- ${target_key} (direct) ---"

  for ep in "${ENDPOINTS[@]}"; do
    ep_safe="${ep//\//-}"
    url="${base_url}/api/${version}/${ep}"

    echo ""
    echo "  Endpoint: /api/${version}/${ep}"

    for cfg in "${CONFIGS[@]}"; do
      IFS=':' read -r threads connections <<< "$cfg"

      # Keep-alive ON (default wrk behavior)
      run_wrk "${target_key}_${ep_safe}_ka-on" "$url" "$threads" "$connections" "false"

      # Keep-alive OFF
      run_wrk "${target_key}_${ep_safe}_ka-off" "$url" "$threads" "$connections" "true"
    done
  done

  echo ""
done

# --- 2. Proxy tests (through nginx) ---

echo "--- Proxy tests (through nginx gateway) ---"

for version in v1 v2 v3 v4 v5; do
  for ep in "${ENDPOINTS[@]}"; do
    ep_safe="${ep//\//-}"
    url="${NGINX}/api/${version}/${ep}"

    echo ""
    echo "  Proxy: /api/${version}/${ep}"

    for cfg in "${CONFIGS[@]}"; do
      IFS=':' read -r threads connections <<< "$cfg"
      run_wrk "proxy_${version}_${ep_safe}" "$url" "$threads" "$connections"
    done
  done
done

echo ""

# --- Summary ---
echo "================================================================"
echo " Benchmark complete. Results saved to: ${RESULT_DIR}"
echo "================================================================"

# Generate summary CSV
SUMMARY="${RESULT_DIR}/summary.csv"
echo "label,threads,connections,rps,latency_avg,latency_p99,errors" > "$SUMMARY"

for f in "${RESULT_DIR}"/*.txt; do
  [ -f "$f" ] || continue
  fname=$(basename "$f" .txt)
  rps=$(grep "Requests/sec" "$f" 2>/dev/null | awk '{print $2}' | head -1 || echo "")
  lat_avg=$(grep "Latency" "$f" 2>/dev/null | head -1 | awk '{print $2}' || echo "")
  lat_p99=$(grep "99%" "$f" 2>/dev/null | awk '{print $2}' | head -1 || echo "")
  errors=$(grep -c "Socket errors\|Non-2xx" "$f" 2>/dev/null || echo "0")
  # Extract t/c from filename
  t_val=$(echo "$fname" | grep -oP 't\K\d+' | head -1 || echo "")
  c_val=$(echo "$fname" | grep -oP 'c\K\d+' | head -1 || echo "")
  label=$(echo "$fname" | sed 's/_t[0-9]*_c[0-9]*//')
  echo "${label},${t_val},${c_val},${rps},${lat_avg},${lat_p99},${errors}" >> "$SUMMARY"
done

echo "Summary CSV: ${SUMMARY}"
