"""Summarize bench_ext runs: cold (pass 1) vs warm (last pass), setup time, per-country speedup spread.
usage: analyze_ext.py OUTDIR baseline_name"""
import re, sys, glob, os, statistics as st, collections
D = sys.argv[1]; base = sys.argv[2]; names = re.findall(r"(\w+)=/", open(D + "/VARIANTS").read())
R = collections.defaultdict(list)   # (ds, name) -> list of trials
for f in sorted(glob.glob(D + "/*-t*-*.stdout")):
    m = re.match(r"(.+)-t(\d+)-(\w+)\.stdout", os.path.basename(f)); ds, t, name = m.group(1), int(m.group(2)), m.group(3)
    out = open(f).read(); tr = dict(setup=float(re.search(r"setup_seconds=([\d.]+)", out)[1]), passes={}, cc={})
    for p, us in re.findall(r"pass=(\d+) us_per_addr=([\d.]+)", out): tr["passes"][int(p)] = float(us)
    for p, cc, n, us in re.findall(r"country pass=(\d+) cc=(\S+) n=(\d+) us_per_addr=([\d.]+)", out): tr["cc"][(int(p), cc)] = (int(n), float(us))
    tr["rss"] = int(re.search(r"peak_rss_bytes=(\d+)", out)[1]); R[(ds, name)].append(tr)
last = max(max(t["passes"]) for v in R.values() for t in v)
print(f"| dataset | variant | trials | setup s (median) | cold pass1 µs/addr | warm pass{last} µs/addr | cold speedup | warm speedup | cold spread | warm spread |\n|---|---|---:|---:|---:|---:|---:|---:|---:|---:|")
for ds in sorted({k[0] for k in R}):
    b1 = st.median(t["passes"][1] for t in R[(ds, base)]); bl = st.median(t["passes"][last] for t in R[(ds, base)])
    for n in names:
        L = R[(ds, n)]; c = [t["passes"][1] for t in L]; w = [t["passes"][last] for t in L]
        print(f"| {ds} | {n} | {len(L)} | {st.median(t['setup'] for t in L):.2f} | {st.median(c):.2f} | {st.median(w):.2f} | {b1/st.median(c):.3f}x | {bl/st.median(w):.3f}x | {100*(max(c)-min(c))/st.median(c):.1f}% | {100*(max(w)-min(w))/st.median(w):.1f}% |")
print("\nPer-country speedup vs baseline (countries with >=100 addresses; medians over trials):")
for ds in sorted({k[0] for k in R}):
    for n in names:
        if n == base: continue
        for p, label in ((1, "cold"), (last, "warm")):
            ratios = []
            ccs = {cc for (pp, cc), (cnt, _) in R[(ds, base)][0]["cc"].items() if pp == p and cnt >= 100}
            for cc in ccs:
                b = st.median(t["cc"][(p, cc)][1] for t in R[(ds, base)]); v = st.median(t["cc"][(p, cc)][1] for t in R[(ds, n)])
                ratios.append((b / v, cc))
            ratios.sort()
            print(f"  {ds:7s} {n:10s} {label}: countries={len(ratios)} min {ratios[0][0]:.3f} ({ratios[0][1]}) median {st.median(r for r,_ in ratios):.3f} max {ratios[-1][0]:.3f} ({ratios[-1][1]}); #countries below 1.05x: {sum(1 for r,_ in ratios if r<1.05)}")
