#!/usr/bin/env bash
set -euo pipefail

DATA="${1:-data/cities.txt}"
GENS="${2:-600}"      # adjust as needed
POP_TOTAL="${3:-800}" # total population across all processes
REPS="${4:-3}"        # repeats per config for smoothing
OUT="${5:-results.csv}"

# header if file doesn't exist
if [ ! -f "$OUT" ]; then
  echo "dataset,p,pop_total,gens,cx,mut,k,mig_int,twoopt,trial,elapsed,best" > "$OUT"
fi

# global GA knobs (match your defaults)
CX="${CX:-0.8}"
MUT="${MUT:-0.05}"
K="${K:-4}"
MIGINT="${MIGINT:-50}"
TWOOPT="${TWOOPT:-1}" # 1=on, 0=off

for P in 1 2 4 8 16 32; do
  # keep TOTAL population constant; scale per-island pop
  POP_ISLAND=$(( POP_TOTAL / P ))
  [ $POP_ISLAND -lt 10 ] && POP_ISLAND=10

  for t in $(seq 1 $REPS); do
    # seed varies by trial for averaging
    SEED=$(( 42 + t ))
    # run
    OUTTXT=$(mpirun -np "$P" ./tsp "$DATA" \
      --generations "$GENS" --pop "$POP_ISLAND" \
      --cx "$CX" --mut "$MUT" --k "$K" \
      --mig-int "$MIGINT" $([ "$TWOOPT" -eq 0 ] && echo --no-twoopt) \
      --seed "$SEED")

    # parse "Elapsed" and "Best tour length"
    ELAPSED=$(echo "$OUTTXT" | sed -n 's/^Elapsed (parallel, p=[0-9][0-9]*): \([0-9.]*\) s/\1/p')
    BEST=$(echo "$OUTTXT" | sed -n 's/^Best tour length: \([0-9.]*\)$/\1/p')

    # fallback parsing (busybox sed quirks)
    if [ -z "$ELAPSED" ]; then ELAPSED=$(echo "$OUTTXT" | grep -Eo 'Elapsed \(parallel, p=[0-9]+\): [0-9.]+ s' | awk '{print $4}'); fi
    if [ -z "$BEST" ]; then BEST=$(echo "$OUTTXT" | grep -Eo '^Best tour length: [0-9.]+' | awk '{print $4}'); fi

    echo "$DATA,$P,$POP_TOTAL,$GENS,$CX,$MUT,$K,$MIGINT,$TWOOPT,$t,$ELAPSED,$BEST" | tee -a "$OUT"
    sleep 0.2
  done
done
