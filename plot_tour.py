#!/usr/bin/env python3
import sys, math
import matplotlib.pyplot as plt

def load_xy(path):
    with open(path,'r') as f:
        n = int(f.readline().strip())
        xs, ys = [], []
        for _ in range(n):
            x,y = map(float, f.readline().split())
            xs.append(x); ys.append(y)
    return xs, ys

def load_perm(path):
    with open(path,'r') as f:
        return [int(x) for x in f.read().strip().split()]

def tour_length(xs, ys, perm):
    L = 0.0; n = len(perm)
    for i in range(n):
        a,b = perm[i], perm[(i+1)%n]
        dx, dy = xs[a]-xs[b], ys[a]-ys[b]
        L += math.hypot(dx, dy)
    return L

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python3 plot_tour.py <dataset.txt> <route.txt> <out.png>")
        sys.exit(1)
    data, route, outpng = sys.argv[1], sys.argv[2], sys.argv[3]
    xs, ys = load_xy(data); perm = load_perm(route)
    L = tour_length(xs, ys, perm)
    px = [xs[i] for i in perm] + [xs[perm[0]]]
    py = [ys[i] for i in perm] + [ys[perm[0]]]
    plt.figure(figsize=(7,6))
    plt.plot(px, py, marker='o', linewidth=1)
    plt.scatter(xs, ys, s=12)
    plt.title(f"{data.split('/')[-1]}: Best Tour (len = {L:.3f})")
    plt.xlabel("x"); plt.ylabel("y"); plt.grid(True, linestyle=':')
    plt.tight_layout(); plt.savefig(outpng, dpi=130)
    print(f"Wrote {outpng} (length={L:.3f})")
