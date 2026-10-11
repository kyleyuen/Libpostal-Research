#!/usr/bin/env python3
"""Verify whole-key and segmented-key diagnostic identities are the same."""
import datetime as dt
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def run(name, argv, raw):
    start = utc()
    locked = [sys.executable, str(ROOT / "research/tools/run_locked.py"), "--", *argv]
    p = subprocess.run(locked, cwd=ROOT, text=True, capture_output=True)
    end = utc()
    rec = {"argv": argv, "locked_argv": locked, "start_utc": start,
           "end_utc": end, "exit_code": p.returncode}
    (raw / f"{name}.command.json").write_text(json.dumps(rec, indent=2) + "\n")
    (raw / f"{name}.stdout").write_text(p.stdout)
    (raw / f"{name}.stderr").write_text(p.stderr)
    (raw / f"{name}.exit").write_text(f"{p.returncode}\n")
    return rec


def main():
    raw = ROOT / "research/analysis/parallel-exploration-2026-10-07/raw/191-diag-identity-calibration"
    raw.mkdir(parents=True, exist_ok=False)
    source = ROOT / "research/analysis/parallel-exploration-2026-10-07/raw/191-diag-identity-calibration.c"
    artifact = ROOT / "research/analysis/parallel-exploration-2026-10-07/derivatives/191/artifacts/diagnostic-v3"
    binary = raw / "calibration"
    metrics = raw / "metrics.json"
    compile_argv = ["/usr/bin/clang", "-std=c11", "-DEXP185_DIAG", "-I", str(artifact.parent.parent / "source"),
                    str(source), str(artifact / "diag.o"), "-o", str(binary)]
    records = [run("compile", compile_argv, raw)]
    if records[-1]["exit_code"]:
        return 1
    records.append(run("execute", [str(binary), str(metrics)], raw))
    if records[-1]["exit_code"]:
        return 1
    data = json.loads(metrics.read_text())
    counters = data["counters"]
    observed = {name: counters[name]["value"] for name in ("lookup_calls", "repeated_queries", "tracked_repeated_queries")}
    expected = {"lookup_calls": 2, "repeated_queries": 1, "tracked_repeated_queries": 1}
    result = {"schema_version": 1, "status": "pass" if observed == expected else "fail",
              "meaning": "same dictionary, operation, and concatenated query bytes match across one whole-key and one segmented event",
              "source_sha256": sha(source), "diagnostic_binary_sha256": sha(artifact / "dump_outputs"),
              "metrics_sha256": sha(metrics), "observed": observed, "expected": expected,
              "commands": records, "ended_utc": utc()}
    (raw / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
