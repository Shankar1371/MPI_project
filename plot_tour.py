#!/usr/bin/env python3
import sys, math
import matplotlib.pyplot as plt

def load_xy(path):
    with open(path, 'r') as f:
        n = int(f.readline().strip())
        xs, ys = [], []
        for _ in range(n):
            x, y = map(float, f.readline().split())
            xs.append(x); ys.append(y)
    return xs, ys

def load_perm(path):
    with open(path, 'r') as f:
        parts = f.read().strip().split()
        return [int(x) for x in parts]

def tour_length(xs, ys, perm):
    n = len(perm)
    L = 0.0
    for i in range(n):
        a, b = perm[i], perm[(i+1)%n]
        dx, dy = xs[a]-xs[b], ys[a]-ys[b]
        L += math.hypot(dx, dy)
    return L

if len(sys.argv) < 4:
    print("Usage: python3 plot_tour.py <dataset.txt> <route.txt> <out.png>")
    sys.exit(1)

data, route_file, out_png = sys.argv[1], sys.argv[2], sys.argv[3]
xs, ys = load_xy(data)
perm = load_perm(route_file)
L = tour_length(xs, ys, perm)

# draw
plt.figure(figsize=(7,6))
# path lines
px = [xs[i] for i in perm] + [xs[perm[0]]]
py = [ys[i] for i in perm] + [ys[perm[0]]]
plt.plot(px, py, marker='o', linewidth=1)

# scatter nodes
plt.scatter(xs, ys, s=12)

# title with length
name = data.split('/')[-1]
plt.title(f"{name}: Best Tour (len = {L:.3f})")
plt.xlabel("x"); plt.ylabel("y"); plt.grid(True, linestyle=':')
plt.tight_layout(); plt.savefig(out_png, dpi=130)
print(f"Wrote {out_png} (length={L:.3f})")
