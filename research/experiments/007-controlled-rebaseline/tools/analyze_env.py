"""Memory and environment summary for a run_controlled.sh session. usage: analyze_env.py OUTDIR"""
import re, sys, glob, os, statistics as st, collections
D = sys.argv[1]; names = re.findall(r"(\w+)=/", open(D + "/VARIANTS").read())
mem = collections.defaultdict(list); env = collections.defaultdict(list)
for f in sorted(glob.glob(D + "/*-t*-*.stderr")):
    ds, t, name = re.match(r"(.+)-t(\d+)-(\w+)\.stderr", os.path.basename(f)).groups(); e = open(f).read()
    mem[name].append((int(re.search(r"(\d+)\s+peak memory footprint", e)[1]), int(re.search(r"(\d+)\s+maximum resident set size", e)[1])))
    try:
        s = open(f[:-6] + "env").read()
        batt = [int(x) for x in re.findall(r"(\d+)%;", s)]; busy = [100 - float(x) for x in re.findall(r"([\d.]+)% idle", s)]
        load = [float(x) for x in re.findall(r"loadavg \{ ([\d.]+)", s)]; swap = [float(x) for x in re.findall(r"used = ([\d.]+)M", s)]
        cpu = [100 - float(x) for x in re.findall(r"([\d.]+)% idle", open(f[:-6] + "cpu").read())[1:]]
        env["batt"] += batt; env["busy_snap"] += busy; env["load"] += load; env["swap_used_mb"] += swap; env["busy_during"] += cpu
    except Exception: pass
print("Peak memory footprint per process (bytes) by variant (all processes, both datasets):\n\n| variant | processes | min | median | max | max RSS max |\n|---|---:|---:|---:|---:|---:|")
for n in names:
    fp = [m[0] for m in mem[n]]; rs = [m[1] for m in mem[n]]
    print(f"| {n} | {len(fp)} | {min(fp):,} | {int(st.median(fp)):,} | {max(fp):,} | {max(rs):,} |")
print("\nEnvironment during the session:")
for k, v in env.items():
    if v: print(f"  {k}: min {min(v):.1f} median {st.median(v):.1f} max {max(v):.1f} (n={len(v)})")
print("  battery health:", open(D + "/battery-health.txt").read().replace("\n", "; ")[:300])
