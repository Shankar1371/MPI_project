import sys

if len(sys.argv) != 3:
    print("Usage: python3 convert_tsplib.py <input.tsp> <output.txt>")
    sys.exit(1)

inp, outp = sys.argv[1], sys.argv[2]
coords = []
with open(inp) as f:
    start = False
    for line in f:
        line = line.strip()
        if "NODE_COORD_SECTION" in line:
            start = True
            continue
        if not start:
            continue
        if "EOF" in line or line == "":
            break
        parts = line.split()
        if len(parts) >= 3:
            coords.append((float(parts[1]), float(parts[2])))

with open(outp, "w") as out:
    out.write(str(len(coords)) + "\n")
    for x, y in coords:
        out.write(f"{x} {y}\n")

print(f"Wrote {len(coords)} points to {outp}")
