import re, glob, os, statistics as st, collections
D = os.path.join(os.path.dirname(__file__), "..", "raw", "subsets")
P = collections.defaultdict(list); N = {}
for f in sorted(glob.glob(D + "/*.stdout")):
    m = re.match(r"(.+)-t(\d+)-(baseline|h)\.stdout", os.path.basename(f)); name, t, v = m.groups()
    out = open(f).read(); mm = re.search(r"corpus=(\d+)", out)
    if not mm or "pass=15" not in out: continue
    N[name] = int(mm[1])
    for p, us in re.findall(r"pass=(\d+) us_per_addr=([\d.]+)", out):
        if int(p) >= 2: P[(name, v)].append(float(us))
def med(x): return st.median(x)
print("| set | addresses | baseline µs/addr | candidate µs/addr | speedup (median) | speedup (P10, interference-resistant) | baseline addr/s | candidate addr/s |\n|---|---:|---:|---:|---:|---:|---:|---:|")
sp = {}
for name in N:
    b, h = P[(name, "baseline")], P[(name, "h")]
    if not b or not h: continue
    p10 = lambda v: sorted(v)[max(0, int(0.1 * len(v)) - 1)]
    sp[name] = med(b) / med(h)
    print(f"| {name} | {N[name]} | {med(b):.2f} | {med(h):.2f} | {med(b)/med(h):.3f}x | {p10(b)/p10(h):.3f}x | {1e6/med(b):,.0f} | {1e6/med(h):,.0f} |")
r = [v for k, v in sp.items() if k.startswith("random_")]
if r: print(f"\nrandom 2,000-address subsets of the real set: n={len(r)} speedup min {min(r):.3f}x median {med(r):.3f}x max {max(r):.3f}x")
