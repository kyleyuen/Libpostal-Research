#!/usr/bin/env python3
"""Gated diagnostic-only supplemental capture against a retained byte oracle."""
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
    ap = argparse.ArgumentParser()
    for name in ("role", "executable", "input", "reference-stdout", "out"):
        ap.add_argument("--" + name, required=True)
    ap.add_argument("--frozen")
    ap.add_argument("--no-metrics", action="store_true")
    a = ap.parse_args()
    out = pathlib.Path(a.out).resolve()
    out.mkdir(parents=True, exist_ok=False)
    inp, exe, reference = map(pathlib.Path, (a.input, a.executable, a.reference_stdout))
    inp, exe, reference = (p.resolve() for p in (inp, exe, reference))
    frozen = pathlib.Path(a.frozen).resolve() if a.frozen else None
    gate_dir = out / "gate"
    gate_dir.mkdir()
    lock_path = ROOT / "research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock"
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    workload_lock = lock_path.open("a+b")
    fcntl.flock(workload_lock.fileno(), fcntl.LOCK_EX)
    gate_start = stamp()
    gate = subprocess.run([str(ROOT / "research/tools/env_gate.sh")], cwd=ROOT, capture_output=True, text=True)
    (gate_dir / "stdout").write_text(gate.stdout)
    (gate_dir / "stderr").write_text(gate.stderr)
    (gate_dir / "exit").write_text(str(gate.returncode) + "\n")
    record = {"schema_version": 1, "role": a.role, "input": str(inp), "input_sha256": sha(inp),
              "input_nonempty_lines": rm.rows(inp), "executable": str(exe), "binary_sha256": sha(exe),
              "reference_stdout": str(reference), "reference_stdout_sha256": sha(reference),
              "frozen_reference": str(frozen) if frozen else None,
              "gate_start": gate_start, "gate_end": stamp(),
              "gate_exit": gate.returncode, "started": stamp()}
    if gate.returncode:
        record["status"] = "gate-failed-parser-not-started"
    else:
        run_dir = out / "capture"
        try:
            samples = rm.capture([str(exe), str(inp)], run_dir,
                                 env=None if a.no_metrics else {"MEASUREMENTS_JSON": str(run_dir / "metrics.json")},
                                 sample_threads=True)
            command = json.loads((run_dir / "command.json").read_text())
            record.update({"capture_command": command, "thread_counts_observed": samples,
                           "stdout": str(run_dir / "stdout"), "stdout_sha256": sha(run_dir / "stdout"),
                           "stderr": str(run_dir / "stderr"), "stderr_sha256": sha(run_dir / "stderr"),
                           "metrics": str(run_dir / "metrics.json") if (run_dir / "metrics.json").exists() else None,
                           "metrics_sha256": sha(run_dir / "metrics.json") if (run_dir / "metrics.json").exists() else None,
                           "byte_identical_to_retained_original": rm.equal_files(run_dir / "stdout", reference),
                           "frozen_check": rm.compare_frozen_dump(run_dir / "stdout", frozen) if frozen else None})
            record["status"] = "pass" if record["byte_identical_to_retained_original"] and (not frozen or record["frozen_check"]["identical"]) and samples and all(n == 1 for n in samples) else "fail"
        except BaseException as exc:
            record["error"] = repr(exc)
            record["status"] = "failed-or-interrupted"
            record["ended"] = stamp()
            (out / "supplement.json").write_text(json.dumps(record, indent=2) + "\n")
            fcntl.flock(workload_lock.fileno(), fcntl.LOCK_UN)
            workload_lock.close()
            raise
    fcntl.flock(workload_lock.fileno(), fcntl.LOCK_UN)
    workload_lock.close()
    record["ended"] = stamp()
    (out / "supplement.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({"status": record["status"], "input_count": record["input_nonempty_lines"], "metrics": record.get("metrics"), "stdout_sha256": record.get("stdout_sha256"), "threads": record.get("thread_counts_observed"), "path": str(out)}, indent=2))
    return 0 if record["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
