#!/usr/bin/env python3
import sys, csv
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print("Usage: python3 plot_speedup.py results.csv [outfile_prefix]")
    raise SystemExit(1)

csv_path = sys.argv[1]
prefix = sys.argv[2] if len(sys.argv) > 2 else ""

ps, times = [], []
with open(csv_path, newline="") as f:
    r = csv.DictReader(f)
    for row in r:
        try:
            ps.append(int(row["p"]))
            times.append(float(row["elapsed"]))
        except Exception:
            pass

if not ps:
    print(f"ERROR: {csv_path} has no valid rows (need 'p,elapsed').")
    raise SystemExit(2)

pairs = sorted(zip(ps, times), key=lambda x: x[0])
ps, times = zip(*pairs)
T1 = dict(pairs).get(1, times[0])

speedup = [T1/t for t in times]
eff = [s/p for s, p in zip(speedup, ps)]

# Speedup
plt.figure()
plt.plot(ps, speedup, marker="o", label="Measured")
plt.plot(ps, ps, linestyle="--", label="Ideal")
plt.title("Speedup vs Processes")
plt.xlabel("Processes (p)"); plt.ylabel("Speedup")
plt.xticks(ps); plt.grid(True, linestyle=":"); plt.legend()
out1 = f"{prefix}speedup.png" if prefix else "speedup.png"
plt.savefig(out1, bbox_inches="tight", dpi=130)

# Efficiency
plt.figure()
plt.plot(ps, eff, marker="o")
plt.title("Parallel Efficiency")
plt.xlabel("Processes (p)"); plt.ylabel("Efficiency = Speedup / p")
plt.xticks(ps); plt.ylim(0, 1.05); plt.grid(True, linestyle=":")
out2 = f"{prefix}efficiency.png" if prefix else "efficiency.png"
plt.savefig(out2, bbox_inches="tight", dpi=130)

print("p,elapsed,Speedup,Efficiency")
for p, t, s, e in zip(ps, times, speedup, eff):
    print(f"{p},{t:.6f},{s:.3f},{e:.3f}")
print(f"Saved: {out1}, {out2}")
