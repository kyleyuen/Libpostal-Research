#!/usr/bin/env python3
"""162: per variant/dataset, cold (pass 1) and warm (median of passes 2..20) us/address per round;
speedups vs cpi and vs original as median of per-round ratios; peak memory from /usr/bin/time -l."""
import re, statistics as st, sys, os, glob
B = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "raw")
REC = 1117339520  # 160 record file, counted in P for b160/c160
V = ["orig", "cpi", "ctl158", "fp158", "fx161", "ctl160", "b160", "c160"]
D = {}
for f in glob.glob(os.path.join(B, "timing", "*.stdout")):
    ds, r, v = re.match(r"(v1|osm)_r(\d)_(\w+)\.stdout", os.path.basename(f)).groups()
    p = {int(m[0]): float(m[1]) for m in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f).read(), re.M)}
    assert len(p) == 20, (f, len(p))
    err = open(f[:-6] + "stderr").read()
    mem = {k: int(re.search(r"(\d+)\s+" + k, err).group(1)) for k in ("maximum resident set size", "peak memory footprint")}
    D[(ds, int(r), v)] = (p[1], st.median(p[k] for k in range(2, 21)), mem)
for ds in ("v1", "osm"):
    print(f"\n== {ds}  (us/addr: median over 5 rounds; speedup = median of per-round ratios)")
    print(f"{'variant':8} {'cold':>7} {'warm':>7} | {'vs cpi cold':>11} {'warm':>6} {'min-max warm':>14} | {'vs orig cold':>12} {'warm':>6} | {'peak fp MB':>10} {'maxRSS MB':>9}")
    for v in V:
        rs = range(1, 6)
        c = [D[(ds, r, v)][0] for r in rs]; w = [D[(ds, r, v)][1] for r in rs]
        rc = [D[(ds, r, "cpi")][0] / D[(ds, r, v)][0] for r in rs]; rw = [D[(ds, r, "cpi")][1] / D[(ds, r, v)][1] for r in rs]
        oc = [D[(ds, r, "orig")][0] / D[(ds, r, v)][0] for r in rs]; ow = [D[(ds, r, "orig")][1] / D[(ds, r, v)][1] for r in rs]
        fp = max(D[(ds, r, v)][2]["peak memory footprint"] for r in rs); rss = max(D[(ds, r, v)][2]["maximum resident set size"] for r in rs)
        print(f"{v:8} {st.median(c):7.3f} {st.median(w):7.3f} | {st.median(rc):11.3f} {st.median(rw):6.3f} {min(rw):6.3f}-{max(rw):6.3f} | {st.median(oc):12.3f} {st.median(ow):6.3f} | {fp/1e6:10.1f} {rss/1e6:9.1f}"
              + (f"  P={(fp+REC)/1e6:.1f}" if v in ("b160", "c160") else ""))
