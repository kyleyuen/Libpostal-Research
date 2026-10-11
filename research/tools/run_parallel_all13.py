#!/usr/bin/env python3
"""Run a candidate's missing observed-workload lanes through the gated checker."""
import argparse
import datetime as dt
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--candidate", required=True)
    ap.add_argument("--diagnostic", required=True)
    ap.add_argument("--out-root", required=True)
    ap.add_argument("--config", default="research/experiments/187-cpi-reuse-flow/config.json")
    ap.add_argument("--skip", action="append", default=[])
    args = ap.parse_args()
    cfg = json.loads((ROOT / args.config).read_text())
    rows = cfg["workload_modes"]["all13"]
    out_root = ROOT / args.out_root
    out_root.mkdir(parents=True, exist_ok=False)
    original = ROOT / "research/experiments/182-benchmark-suite-prep/artifacts/original-000/dump_outputs"
    p185 = ROOT / "research/experiments/185-diagnostic-instrumentation/artifacts/build-20261007-1625-pattern-summary-fix"
    runner = ROOT / "research/tools/run_parallel_candidate_check.py"
    manifest = {"schema_version": 1, "config": str(ROOT / args.config),
                "candidate": str((ROOT / args.candidate).resolve()),
                "diagnostic": str((ROOT / args.diagnostic).resolve()),
                "skipped_names": args.skip,
                "skip_basis": "prior passing candidate lanes are retained separately; skipped names are not implied by this batch",
                "started_utc": utc(), "workloads": []}
    for row in rows:
        name = row["name"]
        if name in args.skip:
            continue
        dest = out_root / name
        argv = [sys.executable, str(runner), "--input", str((ROOT / row["input"]).resolve()),
                "--out", str(dest), "--original", str(original),
                "--original-diagnostic", str(p185 / "original/diagnostic/dump_outputs"),
                "--control", str(p185 / "cpi/release/dump_outputs"),
                "--control-diagnostic", str(p185 / "cpi/diagnostic/dump_outputs"),
                "--candidate", str((ROOT / args.candidate).resolve()),
                "--diagnostic", str((ROOT / args.diagnostic).resolve()),
                "--diagnostic-metrics", "--expect-threads"]
        rec = {"name": name, "input": str((ROOT / row["input"]).resolve()),
               "start_utc": utc(), "argv": argv}
        proc = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True)
        rec.update({"end_utc": utc(), "exit_code": proc.returncode,
                    "stdout": proc.stdout, "stderr": proc.stderr,
                    "result_path": str(dest)})
        manifest["workloads"].append(rec)
        (out_root / "batch.json").write_text(json.dumps(manifest, indent=2) + "\n")
        if proc.returncode:
            manifest["status"] = "fail"
            break
    else:
        manifest["status"] = "pass"
    manifest["ended_utc"] = utc()
    (out_root / "batch.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"status": manifest["status"], "workloads": [
        {"name": x["name"], "exit_code": x["exit_code"], "result_path": x["result_path"]}
        for x in manifest["workloads"]]}, indent=2))
    return 0 if manifest["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
