import pandas as pd
import matplotlib.pyplot as plt
import sys

csv = sys.argv[1] if len(sys.argv) > 1 else "results.csv"
df = pd.read_csv("results.csv")

# Aggregate by p (use mean over trials)
agg = df.groupby("p", as_index=False)["elapsed"].mean().sort_values("p")
# Baseline T1 = mean elapsed when p=1
T1 = agg.loc[agg["p"]==1, "elapsed"].values[0]
agg["speedup"] = T1 / agg["elapsed"]
agg["efficiency"] = agg["speedup"] / agg["p"]

print(agg)

# Speedup plot
plt.figure()
plt.plot(agg["p"], agg["speedup"], marker="o", label="Measured")
plt.plot(agg["p"], agg["p"], linestyle="--", label="Ideal")
plt.xlabel("Processes (p)")
plt.ylabel("Speedup")
plt.title("Speedup vs Processes")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("speedup.png", dpi=160)

# Efficiency plot
plt.figure()
plt.plot(agg["p"], agg["efficiency"], marker="o")
plt.xlabel("Processes (p)")
plt.ylabel("Efficiency = Speedup / p")
plt.title("Parallel Efficiency")
plt.grid(True)
plt.tight_layout()
plt.savefig("efficiency.png", dpi=160)

print("Saved: speedup.png, efficiency.png")
