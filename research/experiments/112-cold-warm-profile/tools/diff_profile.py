"""112: per function, µs per address = (top-of-stack samples / all samples) x (median µs/address of passes >= 2 in that run); cold - warm, median of 2 runs."""
import re, statistics as st, collections
def run(n, r):
    t = open(f"raw/sample_{n}_r{r}.txt").read(); sec = t[t.index("Sort by top of stack"):]
    c = collections.Counter()
    for f, k in re.findall(r"^\s+(\S.*?)\s+\(in [^)]*\)\s+(\d+)\s*$", sec, re.M): c[f] += int(k)
    us = st.median(float(u) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f"raw/bench_{n}_r{r}.stdout").read(), re.M) if int(p) >= 2)
    tot = sum(c.values()); return {f: k / tot * us for f, k in c.items()}, us, tot
R = {n: [run(n, r) for r in (1, 2)] for n in ("cold", "warm")}
for n in R: print(f"{n}: µs/address per run {[round(x[1], 3) for x in R[n]]}, samples {[x[2] for x in R[n]]}")
fs = set().union(*[x[0] for n in R for x in R[n]])
rows = []
for f in fs:
    c = st.median(x[0].get(f, 0) for x in R["cold"]); w = st.median(x[0].get(f, 0) for x in R["warm"]); rows.append((c - w, c, w, f))
rows.sort(reverse=True)
print(f"{'function':45s} {'cold':>7s} {'warm':>7s} {'cold-warm':>9s}")
for d, c, w, f in rows[:25]: print(f"{f[:45]:45s} {c:7.3f} {w:7.3f} {d:9.3f}")
print("sum", round(sum(r[0] for r in rows), 3))
