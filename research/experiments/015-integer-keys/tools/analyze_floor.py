"""Pooled per-pass statistics for raw/floor (passes 2-15 of each process; pass 1 reported separately as cold)."""
import glob, re, statistics as st
R = {}
for f in glob.glob("raw/floor/floor-t*-*.stdout"):
    v = re.search(r"-t\d+-(\w+)\.stdout$", f).group(1)
    ps = [(int(p), float(u)) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f).read(), re.M)]
    r = R.setdefault(v, {"warm": [], "cold": [], "proc_med": []})
    r["cold"] += [u for p, u in ps if p == 1]; w = [u for p, u in ps if p > 1]; r["warm"] += w; r["proc_med"].append(st.median(w))
b = st.median(R["baseline"]["warm"])
print("| variant | processes | warm median µs | P10 µs | per-process medians (min-max) | cold pass1 median µs | speedup vs baseline (warm median) | vs h0 |")
print("|---|---:|---:|---:|---|---:|---:|---:|")
for v in ["baseline", "h0", "k1"]:
    r = R[v]; m = st.median(r["warm"]); p10 = st.quantiles(r["warm"], n=10)[0]
    print(f"| {v} | {len(r['proc_med'])} | {m:.2f} | {p10:.2f} | {min(r['proc_med']):.2f}-{max(r['proc_med']):.2f} | {st.median(r['cold']):.2f} | {b/m:.3f}x | {st.median(R['h0']['warm'])/m:.3f}x |")
