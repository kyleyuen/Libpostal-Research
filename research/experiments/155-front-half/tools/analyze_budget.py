#!/usr/bin/env python3
"""155: apply the pre-registered budget formulas (README) to the one-session measurements. Per round: O, C from bench_ext (median of passes
2..20), split stages (cold) from css, B_new from rec_replay (cold median). Budgets per round, then medians over rounds."""
import re, statistics as st, pathlib
D = pathlib.Path(__file__).resolve().parent.parent / "raw" / "budget"
def bench(t): return st.median(float(u) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", t, re.M) if int(p) >= 2)
def split(t): return {k: float(c) for k, c in re.findall(r"^(\w+) cold_us=([\d.]+)", t, re.M)} | {k + "_w": float(w) for k, w in re.findall(r"^(\w+) cold_us=[\d.]+ warm_us=([\d.]+)", t, re.M)}
def rr(t): m = re.search(r"cold median p2..\d+ ([\d.]+) .*warm ([-\d.]+)", t); return float(m.group(1)), float(m.group(2))
for ds in ("v1", "osm"):
    rows = []
    for r in (1, 2, 3):
        rd = lambda b: (D / f"{b}_{ds}_r{r}.stdout").read_text()
        O, C, S, (B, Bw) = bench(rd("orig")), bench(rd("cpi")), split(rd("css")), rr(rd("rr"))
        s = C / S["total_parse"]; ntr = S["normalize"] + S["tokenize"] + S["response"]; cf = S["context_fill"]
        nec, hyb = O / 8 - s * ntr, O / 8 - s * (ntr + cf)
        Bcpi = s * S["predict_total"]
        rows.append(dict(O=O, C=C, s=s, ntr=s * ntr, cf=s * cf, Bcpi=Bcpi, nec=nec, hyb=hyb, B=B, Bw=Bw, Bcpi_w=S["predict_total_w"]))
        print(f"{ds} r{r}: O {O:.2f} C {C:.3f} ({O/C:.2f}x) s {s:.3f} | cpi B (predict) {Bcpi:.3f} cold, {S['predict_total_w']:.3f} warm(split) | "
              f"budget necessary {nec:.3f} hybrid {hyb:.3f} | B_new cold {B:.3f} warm {Bw:.3f}")
    m = lambda k: st.median(x[k] for x in rows)
    print(f"{ds} MEDIAN: O {m('O'):.2f}  C {m('C'):.3f}  O/8 {m('O')/8:.3f}  cpi: ntr {m('ntr'):.3f} cf {m('cf'):.3f} B {m('Bcpi'):.3f} | "
          f"NECESSARY budget {m('nec'):.3f}  HYBRID budget {m('hyb'):.3f}  | B_new {m('B'):.3f} (warm {m('Bw'):.3f})  -> "
          f"necessary {'PASS' if m('B') <= m('nec') else 'FAIL'}, hybrid {'PASS' if m('B') <= m('hyb') else 'FAIL'}")
