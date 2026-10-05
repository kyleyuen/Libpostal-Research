"""098 screen summary (same format as 097's raw/screen_analyze.stdout): per run, median µs/addr of passes 2-20; per round, base/candidate ratio.
usage: screen_analyze.py SCREEN_DIR BASE CAND [CAND...]"""
import re, sys, glob, statistics as st
D, base, cands = sys.argv[1], sys.argv[2], sys.argv[3:]
def run(ds, n, r):
    p = [float(u) for k, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f"{D}/{ds}_{n}_r{r}.stdout").read(), re.M) if int(k) >= 2]
    assert len(p) == 19, (ds, n, r, len(p)); return round(st.median(p), 3)
for ds in ("v1", "osm"):
    R = {n: [run(ds, n, r) for r in range(1, 7)] for n in [base] + cands}
    for n in [base] + cands: print(f"    {ds} {n} {R[n]} median {round(st.median(R[n]), 3)}")
    for n in cands:
        q = [round(b / c, 4) for b, c in zip(R[base], R[n])]; print(f"    {ds} {n} speedup vs {base} per round {q} median {round(st.median(q), 4)}")
