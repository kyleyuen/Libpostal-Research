import re, glob, statistics as st, os, datetime as dt
D = os.path.join(os.path.dirname(__file__), "..", "raw", "throughput")
rows = {}
for f in sorted(glob.glob(D + "/trial*-*.stdout")):
    tag = os.path.basename(f)[:-7]; trial, split = re.match(r"trial(\d+)-(\w+)", tag).groups()
    out = open(f).read(); err = open(f[:-6] + "stderr").read()
    kv = dict(re.findall(r"(\w+)=(\S+)", out))
    rss = int(re.search(r"(\d+)\s+maximum resident set size", err)[1]); fp = int(re.search(r"(\d+)\s+peak memory footprint", err)[1])
    t0 = dt.datetime.fromisoformat(open(f[:-6] + "start").read().strip()); t1 = dt.datetime.fromisoformat(open(f[:-6] + "end").read().strip())
    ex = re.search(r"exit=(\d+)", err)[1]
    rows.setdefault(split, []).append((int(trial), float(kv["addresses_per_second"]), float(kv["wall_seconds"]), float(kv["cpu_seconds"]),
                                      int(kv["peak_rss_bytes"]), rss, fp, t0, t1, ex, float(kv["us_per_address"])))
print("| Split | Trial | Addresses/s | µs/addr | wall s | CPU s | harness peak RSS | time -l max RSS | time -l peak footprint | run start (PDT) | exit |")
print("|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|")
allrows = sorted((r[7], s, r) for s, l in rows.items() for r in l)
for _, s, r in allrows:
    print(f"| {s} | {r[0]} | {r[1]:,.2f} | {r[10]:.2f} | {r[2]:.3f} | {r[3]:.3f} | {r[4]:,} | {r[5]:,} | {r[6]:,} | {r[7].strftime('%H:%M:%S')} | {r[9]} |")
print()
print("| Split | n | median addr/s | min | max | spread (max-min)/median | CV | median µs/addr | peak RSS min | peak RSS max | RSS range (bytes) | footprint min | footprint max |")
print("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|")
for s in ("development", "validation", "all"):
    l = rows[s]; a = [r[1] for r in l]; m = st.median(a)
    rs = [r[5] for r in l]; fps = [r[6] for r in l]
    print(f"| {s} | {len(l)} | {m:,.2f} | {min(a):,.2f} | {max(a):,.2f} | {100*(max(a)-min(a))/m:.1f}% | {100*st.pstdev(a)/st.mean(a):.2f}% | {st.median(r[10] for r in l):.2f} | {min(rs):,} | {max(rs):,} | {max(rs)-min(rs):,} | {min(fps):,} | {max(fps):,} |")
print()
allr = sorted(r[7] for l in rows.values() for r in l)
ends = sorted((r[7], r[8]) for l in rows.values() for r in l)
gaps = [(ends[i + 1][0] - ends[i][1]).total_seconds() for i in range(len(ends) - 1)]
print(f"inter-run gaps (s): min {min(gaps):.1f} median {st.median(gaps):.1f} max {max(gaps):.1f}; session {ends[0][0].strftime('%H:%M:%S')} - {ends[-1][1].strftime('%H:%M:%S')}")
# drift by trial index across all splits: normalise each split by its median
print("trial-order drift (throughput / split median):")
for s in ("development", "validation", "all"):
    m = st.median(r[1] for r in rows[s]); print(" ", s, " ".join(f"t{r[0]}:{r[1]/m:.3f}" for r in sorted(rows[s])))
