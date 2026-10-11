"""Startup break-even for the 203 candidate, from raw suite timing captures.

For each dataset and round, the original and candidate captures give setup_seconds
(library + model load) and per-pass us_per_addr. Warm cost = median of passes 2..N.
Break-even N solves o.setup + N*o.warm = c.setup + N*c.warm, i.e. the number of
addresses one process must parse before the candidate's slower startup is repaid.
Pass-1 (cold) effects are ignored, so this is the warm-steady-state break-even.
Run from the repository root: python3 -I research/analysis/203-startup-breakeven/breakeven.py
"""
import glob, os, re, statistics as st

RUNS = ["203-stream-gold-novel-v1", "203-stream-osm-v3-v13-v1", "203-stream-v14-v1"]
ORIG, CAND = "original-000", "cpi-stream-203"

caps = {}
for run in RUNS:
    for f in sorted(glob.glob(f"research/suite/results/{run}/timing/*.stdout")):
        ds, var, rd = os.path.basename(f)[:-7].split("__")
        txt = open(f).read()
        us = [float(m) for m in re.findall(r"^pass=\d+ us_per_addr=([\d.]+)", txt, re.M)]
        su = float(re.search(r"setup_seconds=([\d.]+)", txt).group(1))
        caps[(ds, var, rd)] = (su, st.median(us[1:]))

rows = []
for ds in sorted({k[0] for k in caps}):
    for rd in ("r1", "r2", "r3"):
        (os_, ow), (cs, cw) = caps[(ds, ORIG, rd)], caps[(ds, CAND, rd)]
        rows.append((ds, rd, os_, cs, ow, cw, (cs - os_) * 1e6 / (ow - cw)))

print("dataset\tround\torig_setup_s\tcand_setup_s\torig_warm_us\tcand_warm_us\tbreakeven_addresses")
for r in rows:
    print("%s\t%s\t%.3f\t%.3f\t%.3f\t%.3f\t%.0f" % r)
be = [r[6] for r in rows]
print(f"\npairs={len(rows)} setup_median orig={st.median(r[2] for r in rows):.2f}s "
      f"cand={st.median(r[3] for r in rows):.2f}s extra={st.median(r[3]-r[2] for r in rows):.2f}s")
print(f"breakeven addresses: min={min(be):.0f} median={st.median(be):.0f} max={max(be):.0f}")

# METHOD.md defines speedup = median(candidate addr/s) / median(baseline addr/s).
# The suite reports the geometric mean of per-round warm ratios. Report both per dataset.
import math
print("\ndataset\tgeomean_of_round_ratios\tmedian_over_median")
for ds in sorted({k[0] for k in caps}):
    o = [caps[(ds, ORIG, r)][1] for r in ("r1", "r2", "r3")]
    c = [caps[(ds, CAND, r)][1] for r in ("r1", "r2", "r3")]
    g = math.exp(st.mean(math.log(a / b) for a, b in zip(o, c)))
    print(f"{ds}\t{g:.3f}\t{st.median(o) / st.median(c):.3f}")
