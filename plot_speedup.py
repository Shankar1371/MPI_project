import sys
import pandas as pd
import matplotlib.pyplot as plt

if len(sys.argv) != 2 and len(sys.argv) != 3:
    print("Usage: python plot_speedup.py results.csv [prefix]")
    print("If prefix is omitted, it is inferred from the CSV filename.")
    sys.exit(1)

csv_path = sys.argv[1]
if len(sys.argv) == 3:
    prefix = sys.argv[2]
else:
    # strip directory and extension
    import os
    prefix = os.path.splitext(os.path.basename(csv_path))[0]

df = pd.read_csv(csv_path)
if "p" not in df.columns or "elapsed" not in df.columns:
    print("CSV must have columns: p, elapsed")
    sys.exit(1)

df = df.sort_values("p")

# find T1 (time at p=1)
if (df["p"] == 1).sum() == 0:
    print("No row with p=1 found in CSV, cannot compute speedup.")
    sys.exit(1)

T1 = df.loc[df["p"] == 1, "elapsed"].iloc[0]

df["speedup"] = T1 / df["elapsed"]
df["efficiency"] = df["speedup"] / df["p"]

print(df)

# ---- Speedup plot ----
plt.figure()
plt.plot(df["p"], df["speedup"], marker="o")
plt.xlabel("Processes (p)")
plt.ylabel("Speedup S(p)")
plt.title(f"Speedup vs Processes ({prefix})")
plt.grid(True, linestyle="--", alpha=0.5)
plt.xticks(df["p"])
plt.tight_layout()
speedup_name = f"{prefix}_speedup.png"
plt.savefig(speedup_name, dpi=200)
print(f"Saved {speedup_name}")

# ---- Efficiency plot ----
plt.figure()
plt.plot(df["p"], df["efficiency"], marker="o")
plt.xlabel("Processes (p)")
plt.ylabel("Efficiency E(p) = S(p)/p")
plt.title(f"Efficiency vs Processes ({prefix})")
plt.grid(True, linestyle="--", alpha=0.5)
plt.xticks(df["p"])
plt.tight_layout()
eff_name = f"{prefix}_efficiency.png"
plt.savefig(eff_name, dpi=200)
print(f"Saved {eff_name}")
