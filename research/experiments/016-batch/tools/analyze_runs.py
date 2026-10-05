"""Pooled per-pass statistics of bench_ext runs in a directory (files <prefix>-t<n>-<variant>.stdout or r<n>-<variant>.stdout; pass 1 = cold, reported separately).
usage: analyze_runs.py DIR PREFIX BASELINE CONTROL [--paired VARIANT]"""
import glob, re, sys, statistics as st
D, prefix, base, ctl = sys.argv[1:5]; paired = sys.argv[6] if len(sys.argv) > 6 and sys.argv[5] == "--paired" else None
R = {}
for f in sorted(glob.glob(f"{D}/*.stdout")):
    m = re.search(r"(?:-t|r)(\d+)-(\w+)\.stdout$", f)
    if not m: continue
    t, v = int(m.group(1)), m.group(2)
    ps = [(int(p), float(u)) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f).read(), re.M)]
    if not ps: continue
    r = R.setdefault(v, {"warm": [], "cold": [], "proc": {}})
    r["cold"] += [u for p, u in ps if p == 1]; w = [u for p, u in ps if p > 1]; r["warm"] += w; r["proc"][t] = st.median(w)
if paired:
    a, b = R[ctl]["proc"], R[paired]["proc"]; rounds = sorted(set(a) & set(b)); ratios = [a[k] / b[k] for k in rounds]
    print(f"paired rounds={len(rounds)} {paired} faster in {sum(x > 1 for x in ratios)}/{len(rounds)}; median ratio {ctl}/{paired} = {st.median(ratios):.4f}; min {min(ratios):.4f} max {max(ratios):.4f}")
    print(f"medians: {ctl} {st.median(a.values()):.3f} µs, {paired} {st.median(b.values()):.3f} µs"); sys.exit(0)
bm, cm = st.median(R[base]["warm"]), st.median(R[ctl]["warm"])
print("| variant | processes | warm median µs | P10 µs | per-process medians | cold pass1 µs | vs baseline | vs control |")
print("|---|---:|---:|---:|---|---:|---:|---:|")
for v in sorted(R, key=lambda v: -st.median(R[v]["warm"])):
    r = R[v]; m = st.median(r["warm"]); p10 = st.quantiles(r["warm"], n=10)[0]; pm = list(r["proc"].values())
    print(f"| {v} | {len(pm)} | {m:.2f} | {p10:.2f} | {min(pm):.2f}-{max(pm):.2f} | {st.median(r['cold']):.2f} | {bm/m:.3f}x | {cm/m:.3f}x |")
