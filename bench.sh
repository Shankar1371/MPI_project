#!/usr/bin/env bash
set -euo pipefail

DATASET="${1:-data/berlin52.txt}"
GENS="${2:-50}"          # <- cap at 50
TOTAL_POP="${3:-200}"
OUT="${4:-results.csv}"

DATASET="$(readlink -f "$DATASET")"
PROCS=(1 2 4 8)          # keep to your machine; add 16/32 only if needed

echo "p,elapsed" > "$OUT"
for p in "${PROCS[@]}"; do
  POP=$(( TOTAL_POP / p ))
  if [ "$POP" -lt 10 ]; then POP=10; fi
  echo "==> Running p=$p, gens=$GENS, pop/island=$POP"
  mpirun --oversubscribe --wd "$PWD" -np "$p" \
    ./tsp "$DATASET" --generations "$GENS" --pop "$POP" --mig-int 50 \
    | tee _run.txt

  # grab elapsed
  SEC=$(grep -oE 'Elapsed \(parallel, p=[0-9]+\): [0-9.]+ s' _run.txt | awk '{print $(NF-1)}' | tail -n1)
  if [ -z "${SEC:-}" ]; then echo "ERROR: elapsed not found"; exit 1; fi
  echo "$p,$SEC" >> "$OUT"

  # also capture the best tour & length for p=4 (or last run) -> for plotting the route
  if [ "$p" -eq 4 ]; then
    grep -E 'Best tour length:' _run.txt | tail -n1 > best_summary.txt
    grep -E '^Best tour:' _run.txt | tail -n1 | sed 's/^Best tour: //' > best_route.txt
    cp _run.txt last_run.txt
  fi
done
