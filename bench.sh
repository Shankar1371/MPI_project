#!/usr/bin/env bash
set -euo pipefail

# Usage: ./bench.sh <dataset> <generations> <total_pop> <out_csv>
# Example: ./bench.sh data/berlin52.txt 600 200 results.csv
# existing lines...
DATASET="${1:-data/berlin52.txt}"
GENS="${2:-600}"
TOTAL_POP="${3:-200}"
OUT="${4:-results.csv}"

# add this line (normalize to absolute path):
DATASET="$(readlink -f "$DATASET")"

# process counts to test
PROCS=(1 2 4 8 16 32)

# inside bench.sh
# ...
# PROCS=(1 2 4 8 16 32)   # if you want 16/32 as well
PROCS=(1 2 4 8 16 32)

echo "p,elapsed" > "$OUT"
for p in "${PROCS[@]}"; do
  POP=$(( TOTAL_POP / p ))
  if [ "$POP" -lt 10 ]; then POP=10; fi

  echo "==> Running p=$p, gens=$GENS, pop/island=$POP"
  mpirun --oversubscribe --wd "$PWD" -np "$p" \
    ./tsp "$DATASET" --generations "$GENS" --pop "$POP" --mig-int 50 > _run.txt

  SEC=$(grep -oE 'Elapsed \(parallel, p=[0-9]+\): [0-9.]+ s' _run.txt | awk '{print $(NF-1)}')
  if [ -z "$SEC" ]; then echo "ERROR: no elapsed time for p=$p"; cat _run.txt; exit 1; fi
  echo "$p,$SEC" >> "$OUT"
done

