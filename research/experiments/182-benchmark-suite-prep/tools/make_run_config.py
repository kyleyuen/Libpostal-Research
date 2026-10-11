#!/usr/bin/env python3
"""Make an isolated suite config for original 000 vs cpi vs a release candidate.

Usage: python3 make_run_config.py /absolute/path/to/release-harness-dir [output.json]
The release directory must contain bench_ext, dump_outputs, fuzz_pipeline.
No benchmark is started; this only writes the experiment-local config.
"""
import json, os, pathlib, sys

root = pathlib.Path(__file__).resolve().parents[4]
if sys.argv[1:] in (["-h"], ["--help"]):
    print(__doc__)
    raise SystemExit(0)
if len(sys.argv) not in (2, 3):
    raise SystemExit(__doc__)
release = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2]).resolve() if len(sys.argv) == 3 else pathlib.Path(__file__).resolve().parents[1] / "run-suite-3way.json"
required = ("bench_ext", "dump_outputs", "fuzz_pipeline")
for n in required:
    if not (release / n).is_file():
        raise SystemExit(f"release candidate missing {n}: {release / n}")
e = pathlib.Path(__file__).resolve().parents[1]
cfg = json.loads((root / "research/suite/suite.json").read_text())
fuzz_reference = next(v for v in cfg["variants"] if v["name"] == cfg["baseline"]).get("fuzz_reference", {})
cfg["settings"]["correctness_jobs"] = 1
cfg["settings"]["rounds"] = 5
cfg["settings"]["passes"] = 20
cfg["variants"] = [
    {"name": "original-000", "role": "baseline", "enabled": True,
     "desc": "preserved original library from experiments/000-baseline/artifacts/libpostal.a",
     **{n: str(e / "artifacts/original-000" / n) for n in required},
     "fuzz_reference": fuzz_reference},
    {"name": "cpi", "role": "candidate", "enabled": True,
     "desc": "accepted cpi deliverable harnesses",
     **{n: str(root / "research/experiments/119-component-phrase-ids/artifacts/cpi" / n) for n in required}},
    {"name": "release-candidate", "role": "candidate", "enabled": True,
     "desc": f"user-designated release candidate from {release}",
     **{n: str(release / n) for n in required}},
]
cfg["baseline"] = "original-000"
cfg["workspace"] = str(root)
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(cfg, indent=1) + "\n")
print(f"wrote {out}; variants=original-000,cpi,release-candidate; correctness_jobs=1; timing remains serial in run_suite.py")
