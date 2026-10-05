import re, glob, os, sys, statistics as st, datetime as dt
split = sys.argv[1]; D = os.path.join(os.path.dirname(__file__), "..", "raw", "bench-" + split)
names = open(D + "/VARIANTS").read().split(); R = {}
for f in sorted(glob.glob(D + "/round*-*.stdout")):
    rd, name = re.match(r"round(\d+)-(\w+)", os.path.basename(f)).groups(); rd = int(rd)
    out = open(f).read(); err = open(f[:-6] + "stderr").read(); kv = dict(re.findall(r"(\w+)=(\S+)", out))
    R[(rd, name)] = dict(aps=float(kv["addresses_per_second"]), us=float(kv["us_per_address"]), wall=float(kv["wall_seconds"]), cpu=float(kv["cpu_seconds"]),
        rss=int(re.search(r"(\d+)\s+maximum resident set size", err)[1]), fp=int(re.search(r"(\d+)\s+peak memory footprint", err)[1]),
        start=dt.datetime.fromisoformat(open(f[:-6] + "start").read().strip()), ex=re.search(r"exit=(\d+)", err)[1])
rounds = sorted({rd for rd, _ in R if rd > 0})
print(f"## {split}: all trials (round 0 = burn-in, excluded from statistics)\n")
print("| Round | Variant | Addresses/s | µs/addr | wall s | CPU s | max RSS | peak footprint | start | exit |\n|---:|---|---:|---:|---:|---:|---:|---:|---|---|")
for (rd, n), v in sorted(R.items(), key=lambda kv: kv[1]["start"]):
    print(f"| {rd}{' (burn-in)' if rd == 0 else ''} | {n} | {v['aps']:,.2f} | {v['us']:.2f} | {v['wall']:.3f} | {v['cpu']:.3f} | {v['rss']:,} | {v['fp']:,} | {v['start'].strftime('%H:%M:%S')} | {v['ex']} |")
med = {n: st.median(R[(rd, n)]["aps"] for rd in rounds) for n in names}
print("\n| Variant | n | median addr/s | min | max | (max-min)/median | CV | median µs/addr | speedup vs baseline | vs c003 |\n|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|")
for n in names:
    a = [R[(rd, n)]["aps"] for rd in rounds]
    print(f"| {n} | {len(a)} | {med[n]:,.2f} | {min(a):,.2f} | {max(a):,.2f} | {100*(max(a)-min(a))/med[n]:.1f}% | {100*st.pstdev(a)/st.mean(a):.2f}% | {st.median(R[(rd, n)]['us'] for rd in rounds):.2f} | {med[n]/med['baseline']:.4f}x | {(med[n]/med['c003']):.4f}x |" if 'c003' in med else "")
print("\npaired same-round ratios:")
for n in names:
    if n == "baseline": continue
    q = [R[(rd, n)]["aps"] / R[(rd, "baseline")]["aps"] for rd in rounds]
    print(f"  {n}/baseline: " + " ".join(f"{x:.3f}" for x in q) + f" | median {st.median(q):.4f} min {min(q):.4f} max {max(q):.4f}")
    if 'c003' in names and n not in ("c003",):
        q = [R[(rd, n)]["aps"] / R[(rd, "c003")]["aps"] for rd in rounds]
        print(f"  {n}/c003:     " + " ".join(f"{x:.3f}" for x in q) + f" | median {st.median(q):.4f} min {min(q):.4f} max {max(q):.4f}")
for n in names:
    fp = [R[(rd, n)]["fp"] for rd in rounds]; rs = [R[(rd, n)]["rss"] for rd in rounds]
    print(f"  {n}: peak footprint min {min(fp):,} max {max(fp):,} | max RSS min {min(rs):,} max {max(rs):,}")
