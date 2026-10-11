#!/usr/bin/env python3
"""Summarize one benchmark run as PASS/FAIL checks (called by benchmark/run.sh after the suite).

usage: verdict.py RUN_DIR

Reads the suite's raw files in RUN_DIR (timing/*.stdout|stderr|exit, correctness.json, threads.json,
and api/api_parity.json when --api was used) and writes RUN_DIR/verdict.json and RUN_DIR/VERDICT.md.
Checks, all comparing the optimized build with the original on the same inputs:
  correctness  every output dump and fuzz stream byte-identical; the negative control detects a 1-byte change
  speed        every same-round warm ratio > 1 (reported per dataset with the 95% interval)
  memory       optimized peak RSS and peak physical footprint <= original in every same-round pair
  threads      at most one thread in every live sample of the optimized build
  api          (only with --api) every public-API mode identical
Startup cost and the break-even address count are reported, not gated. A positive peak-RSS delta is INCONCLUSIVE
(not PASS) when the original's RSS was below 0.95 x its footprint, i.e. the OS was paging it; rerun with more free
memory. Exit status: 0 PASS, 1 FAIL, 3 INCONCLUSIVE.
"""
import glob, json, os, statistics as st, sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "research", "suite"))
from run_suite import parse_run, t_ci   # noqa: E402  (same parsing and interval as the suite report)


def load(run, name):
    p = os.path.join(run, name)
    return json.load(open(p)) if os.path.exists(p) else None


PAGED = 0.95   # original RSS/footprint below this marks a paged original capture (41 of 42 published 203 original captures: 1.000; one v14 round: 0.836)


def le(a, b):
    return a is not None and b is not None and a <= b


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    run = os.path.abspath(sys.argv[1])
    inv = load(run, "invocation.json") or {}
    base, opt = (inv.get("variants", []) + [None, None])[:2]
    caps = {}
    for f in glob.glob(os.path.join(run, "timing", "*.stdout")):
        ds, var, rd = os.path.basename(f)[:-7].split("__")
        caps[(ds, var, rd)] = parse_run(f[:-7])
    checks, datasets = {}, {}
    c = load(run, "correctness.json")
    if c:
        bad = [k for k, x in c["dumps"].items() if not x["identical"]] + [k for k, x in c["fuzz"].items() if not x["identical"]]
        rows = sum(x["lines"] for x in c["dumps"].values())
        checks["correctness"] = {"pass": not bad and c["negative_control"]["detected"],
                                 "detail": f"{len(c['dumps'])} dump(s) covering {rows:,} addresses and {len(c['fuzz'])} fuzz stream(s) "
                                 + ("byte-identical" if not bad else f"MISMATCH: {', '.join(bad)}")
                                 + ("; negative control detected" if c["negative_control"]["detected"] else "; NEGATIVE CONTROL NOT DETECTED")}
    mem_pairs = mem_bad = mem_unsure = 0; ratios_all = []; mem_notes = []
    for ds in sorted({k[0] for k in caps}):
        rounds = sorted({k[2] for k in caps if k[0] == ds and k[1] == base} & {k[2] for k in caps if k[0] == ds and k[1] == opt})
        pairs = [(caps[(ds, base, r)], caps[(ds, opt, r)]) for r in rounds]
        pairs = [(o, n) for o, n in pairs if o["exit"] == 0 and n["exit"] == 0 and o["warm"] and n["warm"]]
        if not pairs:
            continue
        ratios = [o["warm"] / n["warm"] for o, n in pairs]; ratios_all += ratios
        gm, ci = t_ci(ratios)
        for o, n in pairs:
            mem_pairs += 1
            if not le(n["peak_footprint"], o["peak_footprint"]):
                mem_bad += 1; mem_notes.append(f"{ds}: peak footprint +{n['peak_footprint'] - o['peak_footprint']:,} B")
            elif not le(n["peak_rss"], o["peak_rss"]):
                # METHOD.md: a capture problem that makes the sign uncertain is inconclusive and must be repeated. Only a paged
                # original can make the optimized build look worse: its RSS is then below PAGED x its footprint because the OS
                # compressed or paged out part of it. (A paged optimized process only lowers its own RSS.)
                ratio = o["peak_rss"] / o["peak_footprint"]
                d = f"{ds}: peak RSS +{n['peak_rss'] - o['peak_rss']:,} B"
                if ratio < PAGED:
                    mem_unsure += 1; mem_notes.append(d + f" while the original was paged (its RSS/footprint {ratio:.2f}; footprint lower)")
                else:
                    mem_bad += 1; mem_notes.append(d)
        so, sn = st.median(o["setup_s"] for o, _ in pairs), st.median(n["setup_s"] for _, n in pairs)
        wo, wn = st.median(o["warm"] for o, _ in pairs), st.median(n["warm"] for _, n in pairs)
        be = (sn - so) * 1e6 / (wo - wn) if wn < wo and sn > so else 0
        datasets[ds] = {"rounds": len(pairs), "speedup_geomean": round(gm, 3), "ci95": [round(x, 2) if x else None for x in ci],
                        "speedup_min": round(min(ratios), 3), "speedup_max": round(max(ratios), 3),
                        "original_us_per_address": round(wo, 3), "optimized_us_per_address": round(wn, 3),
                        "original_setup_s": round(so, 2), "optimized_setup_s": round(sn, 2), "breakeven_addresses": round(be),
                        "peak_rss_bytes": {"original": max(o["peak_rss"] or 0 for o, _ in pairs), "optimized": max(n["peak_rss"] or 0 for _, n in pairs)},
                        "peak_footprint_bytes": {"original": max(o["peak_footprint"] or 0 for o, _ in pairs), "optimized": max(n["peak_footprint"] or 0 for _, n in pairs)}}
    if ratios_all:
        checks["speed"] = {"pass": min(ratios_all) > 1, "detail": "; ".join(
            f"{d}: {x['speedup_geomean']:.2f}x (95% CI {x['ci95'][0]}-{x['ci95'][1]}; per round {x['speedup_min']:.2f}-{x['speedup_max']:.2f})" for d, x in datasets.items())}
        checks["memory"] = {"pass": mem_bad == 0 and mem_unsure == 0, "inconclusive": mem_bad == 0 and mem_unsure > 0,
                            "detail": f"optimized peak RSS and peak footprint <= original in {mem_pairs - mem_bad - mem_unsure}/{mem_pairs} same-round pairs"
                            + (f"; {'; '.join(mem_notes)}" if mem_notes else "")
                            + ("; INCONCLUSIVE: the machine was paging memory during the run - free memory (close apps) and rerun" if mem_bad == 0 and mem_unsure else "")}
    th = load(run, "threads.json")
    if th and opt in th:
        live = [x for x in th[opt]["samples"] if x > 0]   # 0 = the process had already finished when sampled
        checks["threads"] = {"pass": bool(live) and max(live) <= 1,
                             "detail": f"optimized: {len(live)} live sample(s), max {max(live) if live else 'none'} thread(s); raw samples {th[opt]['samples']} (0 = process already finished)"}
    api = load(run, os.path.join("api", "api_parity.json"))
    if api:
        failed = [m for m, x in api["modes"].items() if not x["pass"]]
        # The one documented difference (experiment 204): with feature printing on, the optimized build aborts on purpose.
        known = failed == ["print_features"] and api["modes"]["print_features"]["optimized"]["exit"] == -6 and \
            b"EXP_ROW_PACK: only the direct scoring path is supported" in open(os.path.join(run, "api", "print_features.optimized.stderr"), "rb").read()
        checks["api"] = {"pass": api["pass"] or known, "known_limitation": "print_features (experiment 204)" if known else None, "detail": f"{len(api['modes']) - len(failed)}/{len(api['modes'])} public-API modes identical on {api['addresses']:,} addresses"
                         + (f"; FAILED: {', '.join(failed)}" if failed else "")
                         + "".join(f"; {m}: {x['note']}" for m, x in api["modes"].items() if x.get("note"))}
    failed_checks = [k for k, x in checks.items() if not x["pass"] and not x.get("inconclusive")]
    overall = bool(checks) and all(x["pass"] for x in checks.values())
    verdict = "FAIL" if failed_checks or not checks else "PASS" if overall else "INCONCLUSIVE"
    status = lambda x: "INCONCLUSIVE" if x.get("inconclusive") else "FAIL" if not x["pass"] else "KNOWN LIMITATION" if x.get("known_limitation") else "PASS"
    V = {"run": os.path.basename(run), "original": base, "optimized": opt, "overall": verdict, "checks": checks, "datasets": datasets}
    json.dump(V, open(os.path.join(run, "verdict.json"), "w"), indent=1)
    nk = sum(1 for x in checks.values() if x["pass"] and x.get("known_limitation"))
    L = [f"# Verdict: {V['overall']}" + (f" with {nk} known limitation" if overall and nk else ""), "", f"Run `{V['run']}`: `{opt}` (optimized) compared with `{base}` (original libpostal) on the same inputs and machine.", "",
         "| Check | Result | Detail |", "|---|---|---|"]
    L += [f"| {k} | {status(x)} | {x['detail']} |" for k, x in checks.items()]
    if any(x.get("known_limitation") for x in checks.values()):
        L += ["", "KNOWN LIMITATION: `libpostal_parser_print_features(true)` (a debugging switch that prints internal parser features) makes the optimized build stop "
              "with an error by design; every other API family must match. See research/experiments/204-public-api-parity/README.md."]
    if datasets:
        L += ["", "| Dataset | Warm speedup | Original µs/addr | Optimized µs/addr | Startup s (orig / opt) | Break-even addresses |", "|---|---:|---:|---:|---|---:|"]
        L += [f"| {d} | {x['speedup_geomean']:.2f}× | {x['original_us_per_address']} | {x['optimized_us_per_address']} | {x['original_setup_s']} / {x['optimized_setup_s']} | {x['breakeven_addresses']:,} |"
              for d, x in datasets.items()]
        L += ["", "Warm speedup excludes startup. Break-even is how many addresses one process must parse before the optimized build's slower startup is repaid.",
              "Full tables, per-country numbers and raw logs: `report.md`, `timing/`, `env.txt`."]
    open(os.path.join(run, "VERDICT.md"), "w").write("\n".join(L) + "\n")
    print("\n".join(L))
    return {"PASS": 0, "FAIL": 1, "INCONCLUSIVE": 3}[verdict]


if __name__ == "__main__":
    sys.exit(main())
