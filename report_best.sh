#!/usr/bin/env bash
set -euo pipefail

# Datasets (simple XY format: first line N, then N lines "x y")
datasets=(
  "data/pr439.txt"
  "data/berlin52.txt"
  "data/d198.txt"
  "data/pr1002.txt"
)

# Per-dataset population (tweak if you want)
pop_pr439=400
pop_berlin52=200
pop_d198=300
pop_pr1002=800

# Scratch
tmp_out="_run_report.txt"

# run + extract best length for each dataset (gens fixed to 50)
best_pr439=""
best_berlin52=""
best_d198=""
best_pr1002=""

for ds in "${datasets[@]}"; do
  case "$ds" in
    *pr439.txt)   POP=$pop_pr439;   key=pr439      ;;
    *berlin52.txt) POP=$pop_berlin52; key=berlin52 ;;
    *d198.txt)     POP=$pop_d198;     key=d198     ;;
    *pr1002.txt)   POP=$pop_pr1002;   key=pr1002   ;;
  esac

  echo "==> Running $ds (gens=50, pop=$POP)"
  mpirun --oversubscribe --wd "$PWD" -np 4 \
    ./tsp "$ds" --generations 50 --pop "$POP" --mig-int 50 > "$tmp_out"

  len=$(grep -E "Best tour length:" "$tmp_out" | tail -n1 | awk '{print $4}')
  if [ -z "${len:-}" ]; then
    echo "ERROR: couldn't parse best length for $ds"
    tail -n 50 "$tmp_out"; exit 1
  fi

  case "$key" in
    pr439)      best_pr439="$len" ;;
    berlin52)   best_berlin52="$len" ;;
    d198)       best_d198="$len" ;;
    pr1002)     best_pr1002="$len" ;;
  esac
done

# Print banner like your screenshot
cat <<EOF

All,

The attached table shows the state-of-art tour length for each of the following
traveling sales problems.

  pr439.tsp              berlin52.tsp            d198.tsp
  best tour length             7542                    15780
  ${best_pr439}

  pr1002.tsp
  ${best_pr1002}

(Above: your GA results for pr439/pr1002 on this machine; berlin52/d198 optimal values shown for reference.)
EOF
