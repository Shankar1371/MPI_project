#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./bench.sh DATASET TOTAL_POP OUT_CSV
#
# Example:
#   ./bench.sh data/berlin52.txt 400 results_berlin52.csv
#
# Meaning:
#   DATASET   = path to *.txt TSP dataset (berlin52, d198, pr439, pr1002, ...)
#   TOTAL_POP = total population across all processes (kept roughly constant)
#   OUT_CSV   = output CSV file with columns: p,elapsed
#
# For each p in {1,2,4,8,16,32}, we run:
#   pop_per_island = TOTAL_POP / p   (>= 10)
#   gens = 50
# and append "p,elapsed_seconds" to OUT_CSV.

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 DATASET TOTAL_POP OUT_CSV" >&2
  exit 1
fi

DATASET="$1"
TOTAL_POP="$2"
OUT="$3"

# basic validation
if ! [[ "$TOTAL_POP" =~ ^[0-9]+$ ]]; then
  echo "[error] TOTAL_POP must be an integer, got '$TOTAL_POP'" >&2
  exit 1
fi

if [[ ! -f "$DATASET" ]]; then
  echo "[error] dataset '$DATASET' not found" >&2
  exit 1
fi

# fixed generations for this experiment
GENS=50

# processors
PROCS=(1 2 4 8 16 32)

echo "Dataset: $DATASET" >&2
echo "Total population (across all processes): $TOTAL_POP" >&2
echo "Generations: $GENS" >&2
echo

# overwrite CSV and write header
echo "p,elapsed" > "$OUT"

for p in "${PROCS[@]}"; do
  # keep total population roughly constant; divide across islands
  POP=$(( TOTAL_POP / p ))
  (( POP < 10 )) && POP=10   # safety floor

  echo "==> p=$p gens=$GENS pop/island=$POP" >&2

  # run once, log everything to _run.txt and also show on screen
  mpirun --oversubscribe -np "$p" ./tsp "$DATASET" \
    --generations "$GENS" \
    --pop "$POP" \
    --mig-int 50 \
    --cx 0.8 \
    --mut 0.05 \
    --k 4 \
    | tee _run.txt

  # grab the last "Elapsed (parallel, p=...)" line and extract the seconds
  sec=$(grep -oE 'Elapsed \(parallel, p=[0-9]+\): [0-9.]+ s' _run.txt \
        | awk '{print $(NF-1)}' \
        | tail -n 1)

  if [[ -z "${sec:-}" ]]; then
    echo "[error] could not parse elapsed time for p=$p – check _run.txt" >&2
    exit 2
  fi

  echo "  -> elapsed = ${sec} s" >&2
  echo "$p,$sec" >> "$OUT"
  echo >&2
done

echo "Wrote CSV: $OUT"
