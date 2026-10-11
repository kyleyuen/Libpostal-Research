#!/usr/bin/env python3
"""Run the canonical flat-vs-segmented key identity check for 197 v7."""
import datetime as dt
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE = ROOT / "research/analysis/parallel-exploration-2026-10-07"
RAW = BASE / "raw/197-diag-identity-calibration-v7"
SOURCE = BASE / "raw/191-diag-identity-calibration.c"
ARTIFACT = BASE / "derivatives/197/artifacts/diagnostic-v7b"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def run(name, argv):
    start = utc()
    locked = [sys.executable, str(ROOT / "research/tools/run_locked.py"), "--", *argv]
    p = subprocess.run(locked, cwd=ROOT, text=True, capture_output=True)
    end = utc()
    rec = {"argv": argv, "locked_argv": locked, "start_utc": start,
           "end_utc": end, "exit_code": p.returncode}
    (RAW / f"{name}.command.json").write_text(json.dumps(rec, indent=2) + "\n")
    (RAW / f"{name}.stdout").write_text(p.stdout)
    (RAW / f"{name}.stderr").write_text(p.stderr)
    (RAW / f"{name}.exit").write_text(f"{p.returncode}\n")
    return rec


def main():
    RAW.mkdir(parents=True, exist_ok=False)
    binary = RAW / "calibration"
    metrics = RAW / "metrics.json"
    compile_argv = ["/usr/bin/clang", "-std=c11", "-DEXP185_DIAG",
                    "-I", str(ARTIFACT.parent.parent / "source-v7-canonical-diagnostic"),
                    str(SOURCE), str(ARTIFACT / "diag.o"), "-o", str(binary)]
    records = [run("compile", compile_argv)]
    if records[-1]["exit_code"]:
        return 1
    records.append(run("execute", [str(binary), str(metrics)]))
    if records[-1]["exit_code"]:
        return 1
    data = json.loads(metrics.read_text())
    counters = data["counters"]
    observed = {name: counters[name]["value"] for name in
                ("lookup_calls", "repeated_queries", "tracked_repeated_queries")}
    expected = {"lookup_calls": 2, "repeated_queries": 1, "tracked_repeated_queries": 1}
    result = {"schema_version": 1,
              "status": "pass" if observed == expected else "fail",
              "meaning": "same dictionary, operation, and joined query bytes match across a whole-key and a segmented event",
              "source_sha256": sha(SOURCE),
              "diagnostic_binary_sha256": sha(ARTIFACT / "dump_outputs"),
              "diagnostic_archive_sha256": sha(ARTIFACT / "libpostal.a"),
              "metrics_sha256": sha(metrics), "observed": observed,
              "expected": expected, "commands": records, "ended_utc": utc()}
    (RAW / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
