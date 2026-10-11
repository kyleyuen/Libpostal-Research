#!/usr/bin/env python3
"""Sequentially compare safe public-API edge records and retain thread/hash evidence."""
import argparse
import datetime as dt
import fcntl
import hashlib
import importlib.util
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
LOCK = ROOT / "research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock"
GATE = ROOT / "research/tools/env_gate.sh"
HARNESS = ROOT / "research/experiments/186-architecture-benchmark/tools/edge_api_dump.py"
spec = importlib.util.spec_from_file_location("run_measurements", ROOT / "research/tools/run_measurements.py")
rm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rm)


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def sha(path):
    h = hashlib.sha256()
    with pathlib.Path(path).open("rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--baseline", required=True)
    ap.add_argument("--candidate", required=True, action="append", help="NAME=DYLIB; repeatable")
    ap.add_argument("--cases", required=True)
    ap.add_argument("--datadir", required=True)
    ap.add_argument("--model-manifest-sha256", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    out = (ROOT / args.out).resolve()
    out.mkdir(parents=True, exist_ok=False)
    cases = pathlib.Path(args.cases).resolve()
    datadir = pathlib.Path(args.datadir).resolve()
    roles = [("original-000", pathlib.Path(args.baseline).resolve())]
    for value in args.candidate:
        if "=" not in value:
            ap.error("candidate must be NAME=DYLIB")
        name, library = value.split("=", 1)
        roles.append((name, pathlib.Path(library).resolve()))
    manifest = {"schema_version": 1, "started_utc": utc(),
                "kind": "safe_public_api_edge_exact_comparison",
                "case_registry": str(cases), "case_registry_sha256": sha(cases),
                "model_dir": str(datadir), "model_manifest_sha256": args.model_manifest_sha256,
                "harness": str(HARNESS), "harness_sha256": sha(HARNESS), "roles": []}
    failed = False
    with LOCK.open("a+b") as lock:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        for name, library in roles:
            gate_dir = out / (name + "-gate")
            gate_dir.mkdir()
            gate_start = utc()
            gate = subprocess.run([str(GATE)], cwd=ROOT, capture_output=True, text=True)
            (gate_dir / "stdout").write_text(gate.stdout)
            (gate_dir / "stderr").write_text(gate.stderr)
            (gate_dir / "exit").write_text(f"{gate.returncode}\n")
            role = {"name": name, "library": str(library), "library_sha256": sha(library),
                    "gate_start_utc": gate_start, "gate_end_utc": utc(), "gate_exit": gate.returncode}
            if gate.returncode:
                role["error"] = "AC/environment gate failed; parser not invoked"
                manifest["roles"].append(role)
                failed = True
                break
            run_dir = out / name
            output = run_dir / "edge-output.jsonl"
            argv = [sys.executable, str(HARNESS), "--library", str(library), "--datadir", str(datadir),
                    "--cases", str(cases), "--output-jsonl", str(output)]
            try:
                threads = rm.capture(argv, run_dir, sample_threads=True)
                command = json.loads((run_dir / "command.json").read_text())
                role.update({"argv": argv, "command": command, "output": str(output),
                             "output_sha256": sha(output),
                             "output_rows": sum(1 for _ in output.open("rb")),
                             "threads_observed": sorted(set(threads)),
                             "stderr_sha256": sha(run_dir / "stderr")})
                if command["exit"] != 0 or role["output_rows"] != 78 or role["threads_observed"] != [1]:
                    role["error"] = "API harness exit, selected-row count, or single-thread check failed"
                    failed = True
            except Exception as exc:
                role["error"] = repr(exc)
                failed = True
            manifest["roles"].append(role)
            if failed:
                break
    if not failed and len(manifest["roles"]) == len(roles):
        base = pathlib.Path(manifest["roles"][0]["output"]).read_bytes()
        for role in manifest["roles"]:
            role["byte_identical_to_original"] = pathlib.Path(role["output"]).read_bytes() == base
        manifest["all_outputs_identical"] = all(r["byte_identical_to_original"] for r in manifest["roles"])
    else:
        manifest["all_outputs_identical"] = None
    manifest["ended_utc"] = utc()
    manifest["status"] = "pass" if not failed and manifest["all_outputs_identical"] else "fail"
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"status": manifest["status"], "roles": [
        {k: r.get(k) for k in ("name", "output_rows", "output_sha256", "threads_observed", "byte_identical_to_original", "error")}
        for r in manifest["roles"]], "path": str(out)}, indent=2))
    return 0 if manifest["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
