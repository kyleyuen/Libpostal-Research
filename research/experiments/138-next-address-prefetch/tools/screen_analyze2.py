"""105 screen summary with two statistics per run: median and minimum of passes 2-20 (the minimum added 2026-10-03 because external CPU load
drifted within runs; a decision requires both to agree). usage: screen_analyze2.py SCREEN_DIR ROUNDS BASE CAND"""
import re, sys, statistics as st
D, NR, base, cand = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
def run(ds, n, r):
    p = [float(u) for k, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f"{D}/{ds}_{n}_r{r}.stdout").read(), re.M) if int(k) >= 2]
    assert len(p) == 19; return st.median(p), min(p)
for ds in ("v1", "osm"):
    R = {n: [run(ds, n, r) for r in range(1, NR + 1)] for n in (base, cand)}
    for k, lab in ((0, "median"), (1, "min")):
        b = [x[k] for x in R[base]]; c = [x[k] for x in R[cand]]; q = [round(x / y, 4) for x, y in zip(b, c)]
        print(f"    {ds} {lab:6s} {base} {round(st.median(b), 3)} {cand} {round(st.median(c), 3)}; {cand} speedup per round {q} median {round(st.median(q), 4)}; rounds faster {sum(v > 1 for v in q)}/{NR}")
