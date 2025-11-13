#!/usr/bin/env bash
set -euo pipefail

# Usage: ./bench.sh DATASET TOTAL_POP OUT_CSV
# Example: ./bench.sh data/berlin52.txt 400 results_berlin52.csv

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

# we always use 50 generations for this experiment
GENS=50

# processors we want: 1,2,4,8,16,32
PROCS=(1 2 4 8 16 32)

echo "p,elapsed" > "$OUT"

for p in "${PROCS[@]}"; do
  # keep total population roughly constant; divide across islands
  POP=$(( TOTAL_POP / p ))
  (( POP < 10 )) && POP=10   # safety floor

  echo "==> p=$p gens=$GENS pop/island=$POP"
  mpirun --oversubscribe -np "$p" ./tsp "$DATASET" \
    --generations "$GENS" --pop "$POP" --mig-int 50 \
    | tee _run.txt

  # grab the last "Elapsed (parallel...)" line
  sec=$(grep -oE 'Elapsed \(parallel, p=[0-9]+\): [0-9.]+ s' _run.txt \
        | awk '{print $(NF-1)}' \
        | tail -n 1)

  if [[ -z "${sec:-}" ]]; then
    echo "[error] could not parse elapsed time for p=$p" >&2
    exit 2
  fi

  echo "$p,$sec" >> "$OUT"
done

echo "Wrote $OUT"
