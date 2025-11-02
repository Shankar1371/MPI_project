#!/usr/bin/env bash
set -euo pipefail
for f in results_*.csv; do
  base="${f#results_}"; base="${base%.csv}"   # berlin52 / d198 / pr439
  echo "Plotting $f -> ${base}_*.png"
  python3 plot_speedup.py "$f" "${base}_"
done

