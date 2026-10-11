#!/usr/bin/env python3
"""151: same-session, same-corpus table. bench_ext: median us/addr over passes 2..20 per run. replay_kernel: printed cold medians.
Ratios are formed within a round only, then the median over the 3 rounds is reported."""
import re, statistics as st, pathlib
D = pathlib.Path(__file__).resolve().parent.parent / "raw" / "same"
def bench(f):
    v = [float(u) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", f.read_text(), re.M) if int(p) >= 2]
    return st.median(v)
def rk(f):
    t = f.read_text(); out = {}
    for k in ("K0", "K1", "K2"):
        m = re.search(rf"^{k} .*?cold median p2..\d+ ([\d.]+) .*?warm ([-\d.]+)", t, re.M); out[k] = (float(m.group(1)), float(m.group(2)))
    return out
for ds in ("v1", "osm"):
    rows = []
    for r in (1, 2, 3):
        o, c, k = bench(D / f"orig_{ds}_r{r}.stdout"), bench(D / f"cpi_{ds}_r{r}.stdout"), rk(D / f"rk_{ds}_r{r}.stdout")
        rows.append((o, c, k["K0"][0], k["K1"][0], k["K2"][0], k["K2"][1]))
        print(f"{ds} r{r}: orig {o:.3f}  cpi {c:.3f} ({o/c:.2f}x)  K0 {k['K0'][0]:.3f}  K1 {k['K1'][0]:.3f}  K2 cold {k['K2'][0]:.3f} warm {k['K2'][1]:.3f}  "
              f"K2/cpi {k['K2'][0]/c:.2f}  orig/K2 {o/k['K2'][0]:.1f}x")
    med = lambda i: st.median(x[i] for x in rows)
    print(f"{ds} MEDIAN: orig {med(0):.3f}  cpi {med(1):.3f}  cpi speedup {st.median(x[0]/x[1] for x in rows):.2f}x  K0 {med(2):.3f}  K1 {med(3):.3f}  K2 {med(4):.3f} (warm {med(5):.3f})")
