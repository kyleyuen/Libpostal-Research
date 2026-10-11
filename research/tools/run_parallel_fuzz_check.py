#!/usr/bin/env python3
"""Sequential, AC-gated fuzz-output comparison for a release candidate."""
import argparse
import datetime as dt
import filecmp
import fcntl
import hashlib
import importlib.util
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
LOCK = ROOT / "research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock"
ENV_GATE = ROOT / "research/tools/env_gate.sh"
spec = importlib.util.spec_from_file_location("run_measurements", ROOT / "research/tools/run_measurements.py")
rm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rm)


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def sha(path):
    h = hashlib.sha256()
    with pathlib.Path(path).open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--baseline", required=True)
    ap.add_argument("--candidate", required=True)
    ap.add_argument("--seed", required=True, type=int)
    ap.add_argument("--count", required=True, type=int)
    ap.add_argument("--corpus", required=True, action="append")
    ap.add_argument("--model-manifest-sha256", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    if args.count <= 0 or not args.corpus:
        ap.error("count and at least one corpus are required")
    out = (ROOT / args.out).resolve()
    out.mkdir(parents=True, exist_ok=False)
    binaries = {"original-000": pathlib.Path(args.baseline).resolve(),
                "candidate": pathlib.Path(args.candidate).resolve()}
    corpora = [pathlib.Path(p).resolve() for p in args.corpus]
    for p in list(binaries.values()) + corpora:
        if not p.is_file():
            ap.error("missing binary or corpus: " + str(p))
    manifest = {
        "schema_version": 1,
        "kind": "fuzz_correctness_only",
        "seed": args.seed,
        "input_count_per_variant": args.count,
        "model_manifest_sha256": args.model_manifest_sha256,
        "corpora": [{"path": str(p), "sha256": sha(p)} for p in corpora],
        "started_utc": utc(),
        "timing_use": "command provenance only; not a performance measurement",
        "roles": [],
    }
    commands = []
    for role, binary in binaries.items():
        record = {"role": role, "binary": str(binary), "binary_sha256": sha(binary),
                  "gate_start_utc": utc()}
        gate_dir = out / (role + "-gate")
        gate_dir.mkdir()
        gate = subprocess.run([str(ENV_GATE)], cwd=ROOT, capture_output=True, text=True)
        (gate_dir / "stdout").write_text(gate.stdout)
        (gate_dir / "stderr").write_text(gate.stderr)
        (gate_dir / "exit").write_text(f"{gate.returncode}\n")
        record["gate_end_utc"] = utc()
        record["gate_exit"] = gate.returncode
        if gate.returncode:
            record["error"] = "AC/environment gate failed; parser was not invoked"
            manifest["roles"].append(record)
            manifest["status"] = "fail"
            manifest["ended_utc"] = utc()
            (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
            return 1
        run_dir = out / role
        argv = [str(binary), str(args.seed), str(args.count), *map(str, corpora)]
        started = utc()
        with LOCK.open("a+b") as lock:
            fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
            try:
                # Recheck the environment while holding the serial parser lock.
                gate = subprocess.run([str(ENV_GATE)], cwd=ROOT, capture_output=True, text=True)
                gate_dir = out / (role + "-gate-locked")
                gate_dir.mkdir()
                (gate_dir / "stdout").write_text(gate.stdout)
                (gate_dir / "stderr").write_text(gate.stderr)
                (gate_dir / "exit").write_text(f"{gate.returncode}\n")
                if gate.returncode:
                    record["gate_locked_exit"] = gate.returncode
                    record["error"] = "locked AC/environment recheck failed; parser was not invoked"
                    manifest["roles"].append(record)
                    manifest["status"] = "fail"
                    manifest["ended_utc"] = utc()
                    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
                    return 1
                thread_counts = rm.capture(argv, run_dir, sample_threads=True)
            finally:
                fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
        ended = utc()
        stdout = run_dir / "stdout"
        record.update({"argv": argv, "start_utc": started, "end_utc": ended,
                       "exit": json.load((run_dir / "command.json").open())["exit"],
                       "stdout_path": str(stdout), "stdout_sha256": sha(stdout),
                       "stdout_bytes": stdout.stat().st_size,
                       "output_rows": sum(1 for _ in stdout.open("rb")),
                       "threads_observed": sorted(set(thread_counts)),
                       "command_record": str(run_dir / "command.json")})
        manifest["roles"].append(record)
        commands.append({"role": role, "argv": argv, "start_utc": started, "end_utc": ended,
                         "exit": record["exit"]})
        (out / "progress.json").write_text(json.dumps({"roles": manifest["roles"]}, indent=2) + "\n")
        if record["exit"] or record["output_rows"] != args.count or record["threads_observed"] != [1]:
            manifest["status"] = "fail"
            manifest["ended_utc"] = utc()
            (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
            return 1
    left = pathlib.Path(manifest["roles"][0]["stdout_path"])
    right = pathlib.Path(manifest["roles"][1]["stdout_path"])
    manifest["output_byte_identical"] = filecmp.cmp(left, right, shallow=False)
    manifest["output_rows_reconciled"] = all(r["output_rows"] == args.count for r in manifest["roles"])
    manifest["status"] = "pass" if manifest["output_byte_identical"] and manifest["output_rows_reconciled"] else "fail"
    manifest["commands"] = commands
    manifest["ended_utc"] = utc()
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({k: manifest[k] for k in ("status", "seed", "input_count_per_variant",
                                                "output_byte_identical", "output_rows_reconciled",
                                                "roles", "ended_utc")}, indent=2))
    return 0 if manifest["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
