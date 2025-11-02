#!/usr/bin/env bash
set -euo pipefail

# Usage: ./bench.sh <dataset> <generations> <total_pop> <out_csv>
# Example: ./bench.sh data/berlin52.txt 600 200 results.csv
DATASET="${1:-data/berlin52.txt}"
GENS="${2:-600}"
TOTAL_POP="${3:-200}"
OUT="${4:-results.csv}"

# process counts to test
PROCS=(1 2 4 8)

# start fresh
echo "p,elapsed" > "$OUT"

for p in "${PROCS[@]}"; do
  # keep total population roughly constant across p
  POP=$(( TOTAL_POP / p ))
  if [ "$POP" -lt 10 ]; then POP=10; fi

  echo "==> Running p=$p, gens=$GENS, pop/island=$POP"
  # single definitive run per p (keeps 1 row per p for plotting scripts expecting uniqueness)
  mpirun -np "$p" ./tsp "$DATASET" --generations "$GENS" --pop "$POP" --mig-int 50 > _run.txt

  # Extract seconds from: "Elapsed (parallel, p=X): Y.s s"
  SEC=$(grep -oE 'Elapsed \(parallel, p=[0-9]+\): [0-9.]+ s' _run.txt | awk '{print $(NF-1)}')
  if [ -z "$SEC" ]; then
    echo "ERROR: Could not parse elapsed time for p=$p"; cat _run.txt; exit 1
  fi

  echo "$p,$SEC" >> "$OUT"
done

echo "Wrote $OUT"
