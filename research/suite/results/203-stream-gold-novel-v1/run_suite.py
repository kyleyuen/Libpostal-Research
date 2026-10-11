#!/usr/bin/env python3
"""Benchmark suite for the libpostal parser research (see README.md). Python 3.9+, standard library only.

For every dataset ("test suite") it runs the baseline (original libpostal) and each candidate (by default the best version, cpi) and reports
the speedup multiplier per dataset = baseline µs/address ÷ candidate µs/address, with a bootstrap 95% CI over rounds.

Stages (run in this order; pick with --stages):
  integrity    SHA-256 manifest of every dataset, binary and model file (and existence checks).
  correctness  every candidate's dump_outputs vs the baseline's (byte-identical) on every dataset, frozen references where configured,
               fuzz seeds vs the baseline's fuzz outputs. Correctness runs may run in parallel (never during timing).
  timing       serial, gated (research/tools/env_gate.sh), rotating variant order per round, /usr/bin/time -l, environment snapshot per run.
  memory       steady-state footprint (footprint -p after N warm passes) per variant; peak RSS / peak footprint come from the timing runs.
  threads      thread count of each variant while parsing (ps -M).
  report       results.json + report.md from the raw files (also: --report-only RUN_DIR re-analyses an existing run).
"""
import argparse, glob, hashlib, json, math, os, random, re, shutil, statistics as st, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
STAGES = ["integrity", "correctness", "timing", "memory", "threads", "report"]


def now():
    return time.strftime("%Y-%m-%dT%H:%M:%S%z")


def log(run, msg):
    line = f"{now()} {msg}"
    print(line, flush=True)
    with open(os.path.join(run, "run.log"), "a") as f:
        f.write(line + "\n")


_HASH = {}
def sha256(path):
    key = (path, os.path.getsize(path), os.path.getmtime(path))
    if key not in _HASH:
        h = hashlib.sha256()
        with open(path, "rb") as f:
            for b in iter(lambda: f.read(1 << 20), b""):
                h.update(b)
        _HASH[key] = h.hexdigest()
    return _HASH[key]


def load_config(path, args):
    cfg = json.load(open(path))
    ws = cfg["workspace"]
    absp = lambda p: p if os.path.isabs(p) else os.path.join(ws, p)
    s = cfg["settings"]
    if args.rounds: s["rounds"] = args.rounds
    if args.passes: s["passes"] = args.passes
    if args.fuzz_count is not None: s["fuzz_count"] = args.fuzz_count
    if args.no_gate: s["no_gate"] = True
    want_v = set(args.variants.split(",")) if args.variants else None
    want_d = set(args.datasets.split(",")) if args.datasets else None
    variants = []
    for v in cfg["variants"]:
        if v["name"] != cfg["baseline"] and ((want_v is None and not v.get("enabled", True)) or (want_v is not None and v["name"] not in want_v)):
            continue
        for k in ("bench_ext", "dump_outputs", "fuzz_pipeline"):
            if v.get(k): v[k] = absp(v[k])
        v["fuzz_reference"] = {k: absp(p) for k, p in v.get("fuzz_reference", {}).items()}
        variants.append(v)
    datasets = []
    for d in cfg["datasets"]:
        if want_d is not None and d["name"] not in want_d: continue
        if d.get("reserved") and not args.include_reserved: continue
        if want_d is None and not d.get("enabled", True): continue
        for k in ("txt", "tsv", "frozen_reference"):
            if d.get(k): d[k] = absp(d[k])
        datasets.append(d)
    s["fuzz_corpora"] = [absp(p) for p in s["fuzz_corpora"]]
    cfg["variants"], cfg["datasets"] = variants, datasets
    cfg["_abs"] = absp
    return cfg


def run_cmd(cmd, out, err):
    with open(out, "wb") as o, open(err, "wb") as e:
        return subprocess.run(cmd, stdout=o, stderr=e).returncode


# ---------------------------------------------------------------- integrity
def stage_integrity(cfg, run):
    man = {"datasets": {}, "variants": {}, "model": {}, "suite": {}}
    for d in cfg["datasets"]:
        e = {}
        for k in ("txt", "tsv", "frozen_reference"):
            if d.get(k):
                if not os.path.exists(d[k]): raise SystemExit(f"missing {k} for dataset {d['name']}: {d[k]}")
                e[k] = {"path": d[k], "sha256": sha256(d[k])}
        e["addresses"] = sum(1 for l in open(d["txt"], "rb") if l.strip())
        man["datasets"][d["name"]] = e
    for v in cfg["variants"]:
        e = {}
        for k in ("bench_ext", "dump_outputs", "fuzz_pipeline"):
            if v.get(k):
                if not os.path.exists(v[k]): raise SystemExit(f"missing {k} for variant {v['name']}: {v[k]}")
                e[k] = {"path": v[k], "sha256": sha256(v[k])}
        man["variants"][v["name"]] = e
    for p in sorted(glob.glob(os.path.join(cfg["model_dir"], "**", "*"), recursive=True)):
        if os.path.isfile(p): man["model"][os.path.relpath(p, cfg["model_dir"])] = sha256(p)
    for p in ("run_suite.py", "suite.json"):
        man["suite"][p] = sha256(os.path.join(HERE, p))
    json.dump(man, open(os.path.join(run, "manifest.json"), "w"), indent=1)
    log(run, f"integrity: {len(man['datasets'])} datasets ({sum(x['addresses'] for x in man['datasets'].values()):,} addresses), "
             f"{len(man['variants'])} variants, {len(man['model'])} model files hashed")
    return man


# ---------------------------------------------------------------- correctness
def stage_correctness(cfg, run):
    s = cfg["settings"]; base = next(v for v in cfg["variants"] if v["name"] == cfg["baseline"])
    cache = os.path.join(HERE, "results", "cache"); os.makedirs(cache, exist_ok=True)
    cdir = os.path.join(run, "correctness"); os.makedirs(cdir, exist_ok=True)
    jobs, ref, fuzz_ref = [], {}, {}
    for d in cfg["datasets"]:
        bout = os.path.join(cache, f"dump_{sha256(base['dump_outputs'])[:12]}_{sha256(d['txt'])[:12]}.jsonl")
        ref[d["name"]] = bout
        if not os.path.exists(bout):
            jobs.append((f"baseline dump {d['name']}", [base["dump_outputs"], d["txt"]], bout + ".tmp", bout + ".err"))
        for v in cfg["variants"]:
            if v is base: continue
            jobs.append((f"{v['name']} dump {d['name']}", [v["dump_outputs"], d["txt"]],
                         os.path.join(cdir, f"{v['name']}_{d['name']}.jsonl"), os.path.join(cdir, f"{v['name']}_{d['name']}.err")))
    for seed in s["fuzz_seeds"]:
        r = base["fuzz_reference"].get(str(seed))
        if s["fuzz_count"] != 2000000 or r is None or not os.path.exists(r):   # the preserved references are for 2,000,000 inputs
            r = os.path.join(cache, f"fuzz_{sha256(base['fuzz_pipeline'])[:12]}_s{seed}_n{s['fuzz_count']}.txt")
            if not os.path.exists(r):
                jobs.append((f"baseline fuzz s{seed}", [base["fuzz_pipeline"], str(seed), str(s["fuzz_count"])] + s["fuzz_corpora"], r + ".tmp", r + ".err"))
        fuzz_ref[seed] = r
        for v in cfg["variants"]:
            if v is base or not v.get("fuzz_pipeline"): continue
            jobs.append((f"{v['name']} fuzz s{seed}", [v["fuzz_pipeline"], str(seed), str(s["fuzz_count"])] + s["fuzz_corpora"],
                         os.path.join(cdir, f"{v['name']}_fuzz_s{seed}.txt"), os.path.join(cdir, f"{v['name']}_fuzz_s{seed}.err")))

    def go(j):
        label, cmd, out, err = j
        t0 = time.time(); rc = run_cmd(cmd, out, err)
        if out.endswith(".tmp") and rc == 0: os.replace(out, out[:-4])
        return label, rc, time.time() - t0
    log(run, f"correctness: {len(jobs)} jobs, {s['correctness_jobs']} in parallel")
    with ThreadPoolExecutor(max_workers=s["correctness_jobs"]) as ex:
        for label, rc, dt in ex.map(go, jobs):
            log(run, f"  {label}: exit {rc} ({dt:.0f}s)")
            if rc != 0: raise SystemExit(f"correctness job failed: {label}")
    res = {"dumps": {}, "frozen": {}, "fuzz": {}, "negative_control": None}
    frozen_tool = cfg["_abs"]("research/experiments/015-integer-keys/tools/compare_frozen.py")
    for d in cfg["datasets"]:
        bsha = sha256(ref[d["name"]])
        for v in cfg["variants"]:
            if v is base: continue
            f = os.path.join(cdir, f"{v['name']}_{d['name']}.jsonl"); vsha = sha256(f)
            res["dumps"][f"{v['name']}/{d['name']}"] = {"identical": vsha == bsha, "candidate_sha256": vsha, "baseline_sha256": bsha,
                                                       "lines": sum(1 for _ in open(f, "rb"))}
            if d.get("frozen_reference"):
                out = subprocess.run([sys.executable, frozen_tool, f, d["frozen_reference"]], capture_output=True, text=True).stdout.strip().split("\n")[-1]
                res["frozen"][f"{v['name']}/{d['name']}"] = out
    for seed, r in fuzz_ref.items():
        rsha = sha256(r)
        for v in cfg["variants"]:
            if v is base or not v.get("fuzz_pipeline"): continue
            f = os.path.join(cdir, f"{v['name']}_fuzz_s{seed}.txt")
            res["fuzz"][f"{v['name']}/s{seed}"] = {"identical": sha256(f) == rsha, "inputs": s["fuzz_count"]}
    # negative control: a one-byte change to a baseline dump must change the digest the comparison uses
    d0 = cfg["datasets"][0]["name"]; b = bytearray(open(ref[d0], "rb").read()); i = len(b) // 2
    b[i] = (b[i] + 1) % 256 if b[i] != 10 else 32
    res["negative_control"] = {"dataset": d0, "detected": hashlib.sha256(bytes(b)).hexdigest() != sha256(ref[d0])}
    json.dump(res, open(os.path.join(run, "correctness.json"), "w"), indent=1)
    bad = [k for k, x in res["dumps"].items() if not x["identical"]] + [k for k, x in res["fuzz"].items() if not x["identical"]]
    log(run, f"correctness: {len(res['dumps'])} dumps, {len(res['fuzz'])} fuzz runs, mismatches: {bad or 'none'}")
    return res


# ---------------------------------------------------------------- timing
def gate(cfg, run):
    if cfg["settings"].get("no_gate"):
        return
    # A config may select a narrowly scoped, versioned gate for an explicitly
    # authorized environment exception (for example an AC-only waiver). The
    # default remains the normal project gate; --no-gate still skips all gates.
    g = cfg["_abs"](cfg["settings"].get("environment_gate_script", "research/tools/env_gate.sh")); told = False
    while True:
        r = subprocess.run([g], capture_output=True, text=True)
        with open(os.path.join(run, "gate.log"), "a") as f: f.write(f"{now()} {r.stdout.strip()}\n")
        if r.returncode == 0: return
        if cfg["settings"].get("gate_fail_fast"):
            message = r.stdout.strip() or r.stderr.strip() or f"gate exit {r.returncode}"
            log(run, f"environment gate failed; stopping dependent stages: {message}")
            raise SystemExit(message)
        if not told: log(run, f"waiting for the environment gate: {r.stdout.strip()} (rerun with --no-gate to skip)"); told = True
        time.sleep(30)


def settle(cfg, run):
    s = cfg["settings"]; t0 = time.time()
    while os.getloadavg()[0] >= s["settle_load_1min"]:
        if time.time() - t0 > s["settle_max_wait_s"]:
            log(run, f"settle: gave up after {s['settle_max_wait_s']}s (load {os.getloadavg()}); continuing, recorded"); return False
        time.sleep(30)
    log(run, f"settle: load {os.getloadavg()[0]:.2f} < {s['settle_load_1min']} after {time.time() - t0:.0f}s"); return True


def stage_timing(cfg, run):
    s = cfg["settings"]; tdir = os.path.join(run, "timing"); os.makedirs(tdir, exist_ok=True)
    snap = cfg["_abs"]("research/tools/env_snapshot.sh"); vs = cfg["variants"]
    settle(cfg, run)
    n = len(cfg["datasets"]) * len(vs) * s["rounds"]; k = 0
    for r in range(1, s["rounds"] + 1):
        for d in cfg["datasets"]:
            order = vs[r % len(vs):] + vs[:r % len(vs)]          # rotate which variant runs first, per round
            for v in order:
                k += 1; tag = f"{d['name']}__{v['name']}__r{r}"
                gate(cfg, run)
                subprocess.run([snap, os.path.join(run, "env.txt"), tag])
                rc = run_cmd(["/usr/bin/time", "-l", v["bench_ext"], d["tsv"], str(s["passes"])],
                             os.path.join(tdir, tag + ".stdout"), os.path.join(tdir, tag + ".stderr"))
                open(os.path.join(tdir, tag + ".exit"), "w").write(f"{rc}\n")
                log(run, f"timing {k}/{n} {tag} exit {rc} load {os.getloadavg()[0]:.2f}")
                time.sleep(s["sleep_between_runs_s"])


# ---------------------------------------------------------------- memory, threads
def stage_memory(cfg, run):
    s = cfg["settings"]; out = {}
    d = next((x for x in cfg["datasets"] if x["name"] == s["steady_dataset"]), cfg["datasets"][0])
    for v in cfg["variants"]:
        gate(cfg, run)
        f = os.path.join(run, f"steady_{v['name']}.stdout")
        p = subprocess.Popen([v["bench_ext"], d["tsv"], "1000"], stdout=open(f, "w"), stderr=subprocess.STDOUT)
        t0 = time.time()
        while time.time() - t0 < 300 and not re.search(rf"^pass={s['steady_passes_before_sample']} ", open(f).read(), re.M): time.sleep(1)
        fp = subprocess.run(["footprint", "-p", str(p.pid)], capture_output=True, text=True).stdout
        p.kill(); p.wait()
        m = re.search(r"Footprint: (\d+) ([KMG]B)", fp)
        out[v["name"]] = {"steady_footprint": f"{m.group(1)} {m.group(2)}" if m else None, "dataset": d["name"], "after_passes": s["steady_passes_before_sample"]}
        open(os.path.join(run, f"steady_{v['name']}.footprint"), "w").write(fp)
        log(run, f"memory: {v['name']} steady footprint {out[v['name']]['steady_footprint']}")
    json.dump(out, open(os.path.join(run, "memory.json"), "w"), indent=1)


def stage_threads(cfg, run):
    s = cfg["settings"]; out = {}
    d = next((x for x in cfg["datasets"] if x["name"] == s["thread_check_dataset"]), cfg["datasets"][0])
    for v in cfg["variants"]:
        gate(cfg, run)
        p = subprocess.Popen([v["bench_ext"], d["tsv"], "200"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        counts = []
        for _ in range(6):
            time.sleep(3)
            ps = subprocess.run(["ps", "-M", "-p", str(p.pid)], capture_output=True, text=True).stdout.strip().split("\n")
            counts.append(len(ps) - 1)
        p.kill(); p.wait()
        out[v["name"]] = {"samples": counts, "max_threads": max(counts)}
        log(run, f"threads: {v['name']} {counts}")
    json.dump(out, open(os.path.join(run, "threads.json"), "w"), indent=1)


# ---------------------------------------------------------------- analysis
def parse_run(prefix):
    t = open(prefix + ".stdout").read(); e = open(prefix + ".stderr").read()
    passes = {int(p): float(u) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", t, re.M)}
    countries = {}
    if passes:
        last = max(passes)
        for cc, n, u in re.findall(rf"^country pass={last} cc=(\S+) n=(\d+) us_per_addr=([\d.]+)", t, re.M): countries[cc] = (float(u), int(n))
    g = lambda pat: (lambda m: int(m.group(1)) if m else None)(re.search(pat, e))
    setup = re.search(r"setup_seconds=([\d.]+)", t)
    return {"warm": st.median([u for p, u in passes.items() if p >= 2]) if len(passes) > 1 else None, "cold": passes.get(1),
            "setup_s": float(setup.group(1)) if setup else None, "countries": countries,
            "peak_rss": g(r"(\d+)\s+maximum resident set size"), "peak_footprint": g(r"(\d+)\s+peak memory footprint"),
            "exit": int(open(prefix + ".exit").read().strip())}


T975 = [None, 12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228, 2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101,
        2.093, 2.086, 2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042]   # two-sided 95% Student t, by degrees of freedom


def t_ci(ratios):
    """Geometric mean of per-round ratios and its 95% t-interval (computed on log ratios). Rounds are the independent units."""
    n = len(ratios); logs = [math.log(x) for x in ratios]; m = sum(logs) / n
    if n < 2: return math.exp(m), [None, None]
    sd = st.stdev(logs); t = T975[n - 1] if n - 1 < len(T975) else 1.96; h = t * sd / math.sqrt(n)
    return math.exp(m), [math.exp(m - h), math.exp(m + h)]


def boot_ci(ratios, cfg):
    s = cfg["settings"]; rnd = random.Random(s["bootstrap_seed"]); n = len(ratios)
    if n < 2: return [None, None]
    meds = sorted(st.median([rnd.choice(ratios) for _ in range(n)]) for _ in range(s["bootstrap_resamples"]))
    return [meds[int(0.025 * len(meds))], meds[int(0.975 * len(meds)) - 1]]


def geomean(xs):
    xs = [x for x in xs if x]
    return math.exp(sum(math.log(x) for x in xs) / len(xs)) if xs else None


def stage_report(cfg, run):
    s = cfg["settings"]; base = cfg["baseline"]; tdir = os.path.join(run, "timing")
    R = {"run": os.path.basename(run), "generated": now(), "settings": s, "baseline": base, "datasets": {}, "aggregate": {}}
    for d in cfg["datasets"]:
        dd = {"source": d["source"], "kind": d["kind"], "variants": {}, "speedup": {}}
        runs_by_v = {}
        for v in cfg["variants"]:
            runs = {}
            for f in glob.glob(os.path.join(tdir, f"{d['name']}__{v['name']}__r*.stdout")):
                r = int(re.search(r"__r(\d+)\.stdout$", f).group(1)); runs[r] = parse_run(f[:-7])
            ok = {r: x for r, x in runs.items() if x["exit"] == 0 and x["warm"] is not None}
            if not ok: continue
            runs_by_v[v["name"]] = ok
            w = [x["warm"] for x in ok.values()]
            dd["variants"][v["name"]] = {"rounds": len(ok), "warm_median_us": st.median(w), "warm_min_us": min(w), "warm_max_us": max(w),
                                         "cold_median_us": st.median([x["cold"] for x in ok.values()]),
                                         "setup_median_s": st.median([x["setup_s"] for x in ok.values() if x["setup_s"]] or [0]),
                                         "peak_rss_max": max((x["peak_rss"] or 0) for x in ok.values()),
                                         "peak_footprint_max": max((x["peak_footprint"] or 0) for x in ok.values())}
        b = runs_by_v.get(base)
        for vn, vr in runs_by_v.items():
            if vn == base or b is None: continue
            rounds = sorted(set(b) & set(vr))
            if not rounds: continue
            ratios = [b[r]["warm"] / vr[r]["warm"] for r in rounds]
            cold = [b[r]["cold"] / vr[r]["cold"] for r in rounds]
            cc = {}; minn = cfg["settings"].get("country_min_n", 50)
            for c, (u0, n0) in b[rounds[0]]["countries"].items():
                if n0 < minn: continue
                xs = [b[r]["countries"][c][0] / vr[r]["countries"][c][0] for r in rounds if c in b[r]["countries"] and c in vr[r]["countries"]]
                if xs: cc[c] = st.median(xs)
            gm, ci = t_ci(ratios)
            dd["speedup"][vn] = {"median": st.median(ratios), "geomean": gm, "ci95": ci, "ci_method": "t-interval on log per-round ratios",
                                 "bootstrap_median_ci95": boot_ci(ratios, cfg), "min": min(ratios), "max": max(ratios),
                                 "rounds": len(ratios), "per_round": ratios,
                                 "ratio_of_medians": dd["variants"][base]["warm_median_us"] / dd["variants"][vn]["warm_median_us"],
                                 "cold_median": st.median(cold),
                                 "per_country": {"n": len(cc), "min_addresses": minn, "min": min(cc.values()) if cc else None,
                                                 "median": st.median(list(cc.values())) if cc else None, "max": max(cc.values()) if cc else None}}
        R["datasets"][d["name"]] = dd
    for v in cfg["variants"]:
        if v["name"] == base: continue
        sp = {n: x["speedup"][v["name"]]["median"] for n, x in R["datasets"].items() if v["name"] in x["speedup"]}
        if not sp: continue
        by = {}
        for n, x in sp.items(): by.setdefault(R["datasets"][n]["source"], []).append(x)
        bys = {k: geomean(x) for k, x in by.items()}
        R["aggregate"][v["name"]] = {"datasets": len(sp), "source_weighted_geomean": geomean(bys.values()), "geomean_all_datasets": geomean(sp.values()),
                                    "min": min(sp.values()), "max": max(sp.values()), "geomean_by_source": bys,
                                    "datasets_by_source": {k: len(x) for k, x in by.items()}}
    for name in ("correctness", "memory", "threads"):
        p = os.path.join(run, name + ".json")
        if os.path.exists(p): R[name] = json.load(open(p))
    R["manifest"] = "manifest.json" if os.path.exists(os.path.join(run, "manifest.json")) else None
    json.dump(R, open(os.path.join(run, "results.json"), "w"), indent=1)
    write_md(cfg, run, R); log(run, "report: results.json and report.md written")


def format_address_count(value):
    """Format an optional manifest count without assuming the manifest has it."""
    return f"{value:,}" if isinstance(value, int) and not isinstance(value, bool) else "—"


def write_md(cfg, run, R, filename="report.md"):
    base = R["baseline"]; s = R["settings"]
    cands = [v["name"] for v in cfg["variants"] if v["name"] != base]
    L = [f"# Benchmark suite report — {R['run']}", "",
         f"Generated {R['generated']}. Baseline `{base}` vs {', '.join(f'`{c}`' for c in cands)}. {s['rounds']} rounds × {s['passes']} passes per dataset and variant, "
         f"serial, gated, variant order rotated per round. **Speedup (×) = {base} µs/address ÷ candidate µs/address** in the same round "
         f"(warm = median of passes 2–{s['passes']}). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on "
         f"their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. "
         f"Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.", ""]
    for vn in cands:
        a = R["aggregate"].get(vn)
        if a:
            L.append(f"**{vn}: {a['source_weighted_geomean']:.2f}× (geometric mean with each source weighted equally)**; by source: "
                     + ", ".join(f"{k} {x:.2f}× ({a['datasets_by_source'][k]} set{'s' if a['datasets_by_source'][k] > 1 else ''})" for k, x in a["geomean_by_source"].items())
                     + f". Unweighted over all {a['datasets']} datasets: {a['geomean_all_datasets']:.2f}× (range {a['min']:.2f}–{a['max']:.2f}×; dominated by the OSM sets, which share one generator).")
    for vn in cands:
        L += ["", f"## Per dataset: {base} vs {vn}", "",
              f"| dataset | source | addresses | {base} µs | {vn} µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥{cfg['settings'].get('country_min_n', 50)} addr) min/median/max |",
              "|---|---|---:|---:|---:|---:|---|---:|---|---:|---|"]
        man = json.load(open(os.path.join(run, "manifest.json"))) if R.get("manifest") else {"datasets": {}}
        for n, d in R["datasets"].items():
            sp = d["speedup"].get(vn); bv = d["variants"].get(base); cv = d["variants"].get(vn)
            if not (sp and bv and cv): continue
            ci = f"{sp['ci95'][0]:.2f}–{sp['ci95'][1]:.2f}" if sp["ci95"][0] else "—"
            pc = sp["per_country"]
            address_count = man["datasets"].get(n, {}).get("addresses")
            address_count_text = format_address_count(address_count)
            L.append(f"| {n} | {d['source']} | {address_count_text} | {bv['warm_median_us']:.3f} | {cv['warm_median_us']:.3f} | "
                     f"**{sp['geomean']:.2f}×** | {ci} | {sp['median']:.2f}× | {sp['min']:.2f}–{sp['max']:.2f} | {sp['cold_median']:.2f}× | "
                     + (f"{pc['min']:.2f} / {pc['median']:.2f} / {pc['max']:.2f} (n={pc['n']})" if pc["n"] else "—") + " |")
    L += ["", "## Memory and setup", "", "| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |", "|---|---:|---:|---:|---|"]
    mem = R.get("memory", {})
    for v in cfg["variants"]:
        vals = [d["variants"][v["name"]] for d in R["datasets"].values() if v["name"] in d["variants"]]
        if not vals: continue
        L.append(f"| {v['name']} | {max(x['peak_rss_max'] for x in vals):,} | {max(x['peak_footprint_max'] for x in vals):,} | "
                 f"{st.median([x['setup_median_s'] for x in vals]):.2f} | {mem.get(v['name'], {}).get('steady_footprint', '—')} |")
    c = R.get("correctness")
    if c:
        bad = [k for k, x in c["dumps"].items() if not x["identical"]] + [k for k, x in c["fuzz"].items() if not x["identical"]]
        L += ["", "## Correctness (byte-identical to the baseline)", "",
              f"- dumps: {sum(x['identical'] for x in c['dumps'].values())}/{len(c['dumps'])} identical ({sum(x['lines'] for x in c['dumps'].values()):,} parsed addresses compared)",
              "- frozen references: " + ("; ".join(f"{k}: {x}" for k, x in c["frozen"].items()) or "—"),
              "- fuzz: " + ("; ".join(f"{k}: {'identical' if x['identical'] else 'DIFFERENT'} ({x['inputs']:,} inputs)" for k, x in c["fuzz"].items()) or "—"),
              f"- negative control (a one-byte change is detected): {c['negative_control']['detected']}",
              f"- **mismatches: {', '.join(bad) if bad else 'none'}**"]
    th = R.get("threads")
    if th: L += ["", "## Threads", ""] + [f"- {k}: samples {x['samples']} (max {x['max_threads']})" for k, x in th.items()]
    L += ["", "## Limitations", "",
          "- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.",
          "- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`."]
    open(os.path.join(run, filename), "w").write("\n".join(L) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--config", default=os.path.join(HERE, "suite.json"))
    ap.add_argument("--stages", default=",".join(STAGES), help="comma list from: " + ",".join(STAGES))
    ap.add_argument("--datasets", help="comma list of dataset names (default: all enabled, non-reserved)")
    ap.add_argument("--variants", help="comma list of candidate names (baseline always included; default: all enabled)")
    ap.add_argument("--include-reserved", action="store_true", help="also run reserved (never-used) unseen sets: this SPENDS them")
    ap.add_argument("--rounds", type=int); ap.add_argument("--passes", type=int); ap.add_argument("--fuzz-count", type=int)
    ap.add_argument("--no-gate", action="store_true", help="skip research/tools/env_gate.sh before timed runs (recorded in results.json settings)")
    ap.add_argument("--run-id"); ap.add_argument("--report-only", metavar="RUN_DIR", help="re-analyse an existing run directory")
    args = ap.parse_args()
    cfg = load_config(args.config, args)
    if args.report_only:
        stage_report(cfg, os.path.abspath(args.report_only)); return
    run = os.path.join(HERE, "results", args.run_id or time.strftime("%Y%m%d-%H%M%S")); os.makedirs(run, exist_ok=True)
    shutil.copy(args.config, os.path.join(run, "suite.json")); shutil.copy(os.path.abspath(__file__), os.path.join(run, "run_suite.py"))
    json.dump({"argv": sys.argv, "start": now(), "variants": [v["name"] for v in cfg["variants"]], "datasets": [d["name"] for d in cfg["datasets"]]},
              open(os.path.join(run, "invocation.json"), "w"), indent=1)
    for name in STAGES:
        if name not in args.stages.split(","): continue
        log(run, f"=== stage {name}")
        {"integrity": stage_integrity, "correctness": stage_correctness, "timing": stage_timing, "memory": stage_memory,
         "threads": stage_threads, "report": stage_report}[name](cfg, run)
    log(run, f"done: {run}")


if __name__ == "__main__":
    main()
