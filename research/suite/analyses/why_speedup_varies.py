#!/usr/bin/env python3
"""Why does the speedup differ between datasets? Reads one suite run (raw timing files) and its suite.json.
1. Per dataset: original µs, cpi µs, speedup, and what the addresses look like (characters, words, non-ASCII share).
2. Pooled over every (dataset, country) cell with >= MIN_N addresses: fit cpi_us = c0 + c1 * original_us (least squares).
   If c0 > 0 the speedup  original/cpi = x / (c0 + c1 x)  rises with how expensive an address is (x), towards 1/c1.
3. Predict each dataset's speedup from its original µs with that one line, and compare with the measured speedup.
usage: why_speedup_varies.py RUN_DIR [CANDIDATE] [MIN_N]"""
import glob, json, os, re, statistics as st, sys
run = sys.argv[1]; cand = sys.argv[2] if len(sys.argv) > 2 else "cpi"; MIN_N = int(sys.argv[3]) if len(sys.argv) > 3 else 50
cfg = json.load(open(os.path.join(run, "suite.json"))); ws = cfg["workspace"]; base = cfg["baseline"]
def runs(ds, v):
    out = []
    for f in sorted(glob.glob(os.path.join(run, "timing", f"{ds}__{v}__r*.stdout"))):
        t = open(f).read(); p = {int(a): float(b) for a, b in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", t, re.M)}; last = max(p)
        out.append((st.median([u for k, u in p.items() if k >= 2]), {c: (float(u), int(n)) for c, n, u in re.findall(rf"^country pass={last} cc=(\S+) n=(\d+) us_per_addr=([\d.]+)", t, re.M)}))
    return out
rows, cells = [], []
names = [d["name"] for d in json.load(open(os.path.join(run, "invocation.json")))["datasets"]] if False else json.load(open(os.path.join(run, "invocation.json")))["datasets"]
for d in cfg["datasets"]:
    if d["name"] not in names: continue
    B, C = runs(d["name"], base), runs(d["name"], cand)
    if not B or not C: continue
    ob, oc = st.median(x[0] for x in B), st.median(x[0] for x in C)
    addrs = [l.rstrip("\n") for l in open(os.path.join(ws, d["txt"]), encoding="utf-8", errors="replace") if l.strip()]
    rows.append(dict(name=d["name"], source=d["source"], orig=ob, cpi=oc, speedup=ob / oc, chars=st.mean(len(a) for a in addrs),
                     words=st.mean(len(a.split()) for a in addrs), nonascii=100 * sum(any(ord(ch) > 127 for ch in a) for a in addrs) / len(addrs),
                     digits=100 * st.mean(sum(ch.isdigit() for ch in a) / max(1, len(a)) for a in addrs)))
    for c in B[0][1]:
        if B[0][1][c][1] < MIN_N: continue
        xb = st.median(r[1][c][0] for r in B if c in r[1]); xc = st.median(r[1][c][0] for r in C if c in r[1])
        cells.append((d["name"], c, B[0][1][c][1], xb, xc))
print(f"1. Datasets ({base} vs {cand}; warm µs/address = median over rounds)\n")
print(f"{'dataset':12} {'source':14} {'orig µs':>8} {cand+' µs':>8} {'speedup':>8} {'chars':>6} {'words':>6} {'non-ASCII%':>10} {'digit%':>7}")
for r in rows: print(f"{r['name']:12} {r['source']:14} {r['orig']:8.3f} {r['cpi']:8.3f} {r['speedup']:7.2f}x {r['chars']:6.1f} {r['words']:6.2f} {r['nonascii']:10.1f} {r['digits']:7.1f}")
xs = [c[3] for c in cells]; ys = [c[4] for c in cells]; n = len(xs); mx, my = st.mean(xs), st.mean(ys)
c1 = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs); c0 = my - c1 * mx
r2 = 1 - sum((y - (c0 + c1 * x)) ** 2 for x, y in zip(xs, ys)) / sum((y - my) ** 2 for y in ys)
print(f"\n2. Fit over {n} (dataset, country) cells with >= {MIN_N} addresses:  {cand}_us = {c0:.3f} + {c1:.4f} * original_us   (R^2 = {r2:.3f})")
print(f"   => speedup(x) = x / ({c0:.3f} + {c1:.4f} x): {c0:.3f} µs of {cand}'s time per address does not shrink with address cost; the rest is {100*c1:.1f}% of the original's.")
print(f"   => ceiling for very expensive addresses 1/c1 = {1/c1:.2f}x; speedup at original 10 / 18 / 32 / 50 µs: " + ", ".join(f"{x / (c0 + c1 * x):.2f}x" for x in (10, 18, 32, 50)))
print(f"\n3. Predicted vs measured speedup per dataset (prediction uses only the dataset's original µs and the one line above)\n")
for r in rows:
    p = r["orig"] / (c0 + c1 * r["orig"]); print(f"   {r['name']:12} orig {r['orig']:6.2f} µs  predicted {p:5.2f}x  measured {r['speedup']:5.2f}x  residual {r['speedup'] - p:+.2f}")
cs = sorted(cells, key=lambda c: c[3])
print(f"\n4. Country cells sorted by original µs (cheapest and most expensive five): speedup rises with address cost")
for c in cs[:5] + cs[-5:]: print(f"   {c[0]:12} {c[1]:3} n={c[2]:5}  orig {c[3]:6.2f} µs  {cand} {c[4]:5.2f} µs  speedup {c[3]/c[4]:5.2f}x")
