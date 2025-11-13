import sys
import pandas as pd
import matplotlib.pyplot as plt

if len(sys.argv) < 4:
    print("Usage: plot_gen_time.py results.csv gens out_prefix")
    print("Example: plot_gen_time.py results_berlin52.csv 50 berlin52")
    sys.exit(1)

csv_path = sys.argv[1]
gens = float(sys.argv[2])
prefix = sys.argv[3]

df = pd.read_csv(csv_path)
df = df.sort_values("p")

# time per generation = total elapsed / number of generations
df["gen_time"] = df["elapsed"] / gens

print(df)

plt.figure(figsize=(6,4))
plt.plot(df["p"], df["gen_time"], marker="o", linestyle="-")
plt.xlabel("Processes (p)")
plt.ylabel("Time per generation (seconds)")
plt.title(f"Generation time vs processes ({prefix})")
plt.grid(True, linestyle="--", alpha=0.5)
plt.xticks(df["p"])

plt.tight_layout()
plt.savefig(f"{prefix}_gen_time.png", dpi=200)
print(f"Saved {prefix}_gen_time.png")
