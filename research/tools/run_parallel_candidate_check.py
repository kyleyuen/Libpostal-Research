#!/usr/bin/env python3
"""Sequential, gated byte-output and diagnostic check for parallel candidates."""
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
spec = importlib.util.spec_from_file_location("run_measurements", ROOT / "research/tools/run_measurements.py")
rm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rm)


def sha(path):
    h = hashlib.sha256()
    with pathlib.Path(path).open("rb") as f:
        for b in iter(lambda: f.read(1024 * 1024), b""):
            h.update(b)
    return h.hexdigest()


def stamp():
    return dt.datetime.now().astimezone().isoformat()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--input", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--original", required=True)
    p.add_argument("--control", required=True)
    p.add_argument("--original-diagnostic")
    p.add_argument("--control-diagnostic")
    p.add_argument("--candidate", required=True)
    p.add_argument("--diagnostic")
    p.add_argument("--diagnostic-metrics", action="store_true")
    p.add_argument("--expect-threads", action="store_true")
    a = p.parse_args()
    out = pathlib.Path(a.out).resolve()
    out.mkdir(parents=True, exist_ok=False)
    inp = pathlib.Path(a.input).resolve()
    roles = [("original-000", a.original)]
    if a.original_diagnostic:
        roles.append(("original-diagnostic", a.original_diagnostic))
    roles.append(("119-source-o2-control", a.control))
    if a.control_diagnostic:
        roles.append(("119-source-o2-diagnostic-control", a.control_diagnostic))
    roles.append(("candidate-release", a.candidate))
    if a.diagnostic:
        roles.append(("candidate-diagnostic", a.diagnostic))
    elif a.diagnostic_metrics:
        p.error("--diagnostic is required with --diagnostic-metrics")
    manifest = {"schema_version": 1, "input": str(inp), "input_sha256": sha(inp),
                "input_nonempty_lines": rm.rows(inp), "started": stamp(), "roles": []}
    failed = False
    lock_path = ROOT / "research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock"
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    with lock_path.open("a+b") as workload_lock:
      fcntl.flock(workload_lock.fileno(), fcntl.LOCK_EX)
      for name, executable in roles:
        gate_dir = out / (name + "-gate")
        gate_dir.mkdir()
        gate_start = stamp()
        gate = subprocess.run([str(ROOT / "research/tools/env_gate.sh")], cwd=ROOT,
                              capture_output=True, text=True)
        (gate_dir / "stdout").write_text(gate.stdout)
        (gate_dir / "stderr").write_text(gate.stderr)
        (gate_dir / "exit").write_text(str(gate.returncode) + "\n")
        role = {"name": name, "executable": str(pathlib.Path(executable).resolve()),
                "binary_sha256": sha(executable), "gate_start": gate_start,
                "gate_end": stamp(), "gate_exit": gate.returncode}
        if gate.returncode:
            role["error"] = "AC/environment gate failed; parser not started"
            manifest["roles"].append(role)
            failed = True
            break
        run_dir = out / name
        try:
            env = ({"MEASUREMENTS_JSON": str(run_dir / "metrics.json")}
                   if a.diagnostic_metrics and "diagnostic" in name else None)
            counts = rm.capture([str(pathlib.Path(executable).resolve()), str(inp)], run_dir,
                                env=env, sample_threads=True)
            record = json.loads((run_dir / "command.json").read_text())
            role["record"] = record
            role["stdout_sha256"] = sha(run_dir / "stdout")
            role["stdout_bytes"] = (run_dir / "stdout").stat().st_size
            role["stderr_sha256"] = sha(run_dir / "stderr")
            role["stdout_path"] = str(run_dir / "stdout")
            role["threads_observed"] = sorted(set(counts))
            if (run_dir / "metrics.json").exists():
                role["metrics_path"] = str(run_dir / "metrics.json")
                role["metrics_sha256"] = sha(run_dir / "metrics.json")
        except Exception as exc:
            role["error"] = repr(exc)
            failed = True
        manifest["roles"].append(role)
        if failed:
            break
    if not failed and len(manifest["roles"]) == len(roles):
        ref = pathlib.Path(manifest["roles"][0]["stdout_path"])
        for role in manifest["roles"]:
            role["byte_identical_to_original"] = rm.equal_files(ref, pathlib.Path(role["stdout_path"]))
        manifest["all_outputs_identical"] = all(x["byte_identical_to_original"] for x in manifest["roles"])
        if a.expect_threads:
            manifest["single_thread_verified"] = all(x.get("threads_observed") == [1] for x in manifest["roles"])
        else:
            manifest["single_thread_verified"] = None
    else:
        manifest["all_outputs_identical"] = None
        manifest["single_thread_verified"] = None
    manifest["ended"] = stamp()
    manifest["status"] = "pass" if not failed and manifest.get("all_outputs_identical") and (not a.expect_threads or manifest.get("single_thread_verified")) else "fail"
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"status": manifest["status"], "roles": [(x["name"], x.get("record", {}).get("exit") if isinstance(x.get("record"), dict) else None, x.get("threads_observed"), x.get("byte_identical_to_original")) for x in manifest["roles"]], "path": str(out)}, indent=2))
    return 0 if manifest["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
