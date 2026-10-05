"""Interference-aware analysis of run_quiet.sh output. usage: analyze_quiet.py OUTDIR baseline_name
Per (dataset, variant): pooled warm passes (pass>=2) -> median, P10 (fast tail = interference-free estimate), CPU-time median, 'clean' passes
(no involuntary context switch and wall <= 1.02 x CPU time); cold pass 1 median; system busy % during the runs; speedups vs baseline for each estimator."""
import re, sys, glob, os, statistics as st, collections
D = sys.argv[1]; base = sys.argv[2]; names = [entry.split('=', 1)[0] for entry in open(D + '/VARIANTS').read().split()]
P = collections.defaultdict(list); COLD = collections.defaultdict(list); BUSY = collections.defaultdict(list); SETUP = collections.defaultdict(list); CC = collections.defaultdict(list)
for f in sorted(glob.glob(D + "/*-t*-*.stdout")):
    m = re.match(r"(.+)-t(\d+)-(\w+)\.stdout", os.path.basename(f)); ds, t, name = m.group(1), int(m.group(2)), m.group(3)
    out = open(f).read(); SETUP[(ds, name)].append(float(re.search(r"setup_seconds=([\d.]+)", out)[1]))
    passes = re.findall(r"pass=(\d+) us_per_addr=([\d.]+) addresses_per_second=[\d.]+ cpu_us_per_addr=([\d.]+) nivcsw=(\d+) nvcsw=(\d+)", out)
    last = max(int(p[0]) for p in passes)
    for p, us, cpu, niv, nv in passes:
        rec = (float(us), float(cpu), int(niv))
        if int(p) == 1: COLD[(ds, name)].append(rec[0])
        else: P[(ds, name)].append(rec)
    for cc, n, us in re.findall(r"country pass=%d cc=(\S+) n=(\d+) us_per_addr=([\d.]+)" % last, out): CC[(ds, name, cc)].append((int(n), float(us)))
    try:
        idle = [float(x) for x in re.findall(r"([\d.]+)% idle", open(f[:-6] + "cpu").read())][1:]
        if idle: BUSY[(ds, name)].append(100 - st.mean(idle))
    except Exception: pass
def p10(v): s = sorted(v); return s[max(0, int(0.10 * len(s)) - 1)]
rows = {}
for ds in sorted({k[0] for k in P}):
    for n in names:
        L = P[(ds, n)]; wall = [x[0] for x in L]; cpu = [x[1] for x in L]
        clean = [x[0] for x in L if x[2] == 0 and x[0] <= 1.02 * x[1]]
        rows[(ds, n)] = dict(med=st.median(wall), p10=p10(wall), mn=min(wall), cpu=st.median(cpu), clean=(st.median(clean) if clean else float("nan")), nclean=len(clean), n=len(L),
                             cold=st.median(COLD[(ds, n)]), setup=st.median(SETUP[(ds, n)]), busy=(st.mean(BUSY[(ds, n)]) if BUSY[(ds, n)] else float("nan")),
                             inter=100 * sum(1 for x in L if x[2] > 0) / len(L))
print("| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |\n|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|")
for (ds, n), r in rows.items():
    print(f"| {ds} | {n} | {r['n']} | {r['setup']:.2f} | {r['busy']:.0f} | {r['inter']:.0f}% | {r['med']:.2f} | {r['p10']:.2f} | {r['cpu']:.2f} | {r['clean']:.2f} ({r['nclean']}) | {r['cold']:.2f} |")
print("\nSpeedup vs baseline (baseline µs / variant µs) by estimator:\n\n| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |\n|---|---|---:|---:|---:|---:|---:|")
for ds in sorted({k[0] for k in P}):
    b = rows[(ds, base)]
    for n in names:
        if n == base: continue
        r = rows[(ds, n)]; print(f"| {ds} | {n} | {b['med']/r['med']:.3f}x | {b['p10']/r['p10']:.3f}x | {b['cpu']/r['cpu']:.3f}x | {b['clean']/r['clean']:.3f}x | {b['cold']/r['cold']:.3f}x |")
print("\nPer-country speedup (last warm pass, median over processes; countries with >=100 addresses):")
for ds in sorted({k[0] for k in P}):
    for n in names:
        if n == base: continue
        rat = []
        for cc in {k[2] for k in CC if k[0] == ds and k[1] == base}:
            bb = CC[(ds, base, cc)]; vv = CC[(ds, n, cc)]
            if bb and vv and bb[0][0] >= 100: rat.append((st.median(x[1] for x in bb) / st.median(x[1] for x in vv), cc))
        rat.sort(); print(f"  {ds:7s} {n:6s}: countries={len(rat)} min {rat[0][0]:.3f} ({rat[0][1]}) median {st.median(r for r,_ in rat):.3f} max {rat[-1][0]:.3f} ({rat[-1][1]}); below 1.05x: {sum(1 for r,_ in rat if r<1.05)}")
