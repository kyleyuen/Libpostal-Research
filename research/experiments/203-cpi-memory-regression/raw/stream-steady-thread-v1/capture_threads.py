#!/usr/bin/env python3
"""Capture six live OS-thread samples for each pinned 203 comparison role."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[5]
OUT = Path(__file__).resolve().parent / "thread-captures"
CONFIGS = [
    ("research/experiments/203-cpi-memory-regression/raw/stream-timing-v1/gold-novel.json", "golden-v1"),
    ("research/experiments/203-cpi-memory-regression/raw/stream-timing-v1/osm-v3-v13.json", "osm-v3"),
    ("research/experiments/203-cpi-memory-regression/raw/stream-timing-v1/v14.json", "unseen-v14"),
]
SAMPLES = 6
INTERVAL_S = 3
PASSES = 5000
READY_PASS = 50


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def read_json(path):
    return json.loads((ROOT / path).read_text())


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    gate = ROOT / "research/tools/env_gate.sh"
    snap = ROOT / "research/tools/env_snapshot.sh"
    results = {"passes": PASSES, "ready_pass": READY_PASS, "samples": SAMPLES,
               "interval_seconds": INTERVAL_S, "records": []}
    for config_path, dataset_name in CONFIGS:
        cfg = read_json(config_path)
        dataset = next(d for d in cfg["datasets"] if d["name"] == dataset_name)
        tsv = ROOT / dataset["tsv"]
        for role in ("original-000", "cpi-stream-203"):
            variant = next(v for v in cfg["variants"] if v["name"] == role)
            binary = ROOT / variant["bench_ext"]
            tag = f"{dataset_name}__{role}"
            stdout_path = OUT / f"{tag}.stdout"
            stderr_path = OUT / f"{tag}.stderr"
            snapshot_path = OUT / "environment.txt"
            gate_run = subprocess.run([str(gate)], cwd=ROOT, text=True,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            with open(OUT / "gate.log", "a") as f:
                f.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')} {tag} {gate_run.stdout.strip()}\n")
            if gate_run.returncode:
                raise SystemExit(f"environment gate failed for {tag}: {gate_run.stdout.strip()}")
            subprocess.run([str(snap), str(snapshot_path), tag], cwd=ROOT, check=True)
            env = os.environ.copy()
            env["PC_COMPRESS_VERBOSE"] = "1"
            rec = {"tag": tag, "dataset": dataset_name, "role": role,
                   "command": [str(binary), str(tsv), str(PASSES)],
                   "binary_sha256": sha256(binary), "tsv_sha256": sha256(tsv),
                   "stdout": str(stdout_path.relative_to(ROOT)),
                   "stderr": str(stderr_path.relative_to(ROOT)),
                   "gate_exit": gate_run.returncode, "ready_pass": READY_PASS,
                   "samples": [], "termination": None}
            with open(stdout_path, "w") as stdout, open(stderr_path, "w") as stderr:
                p = subprocess.Popen(rec["command"], cwd=ROOT, env=env, stdout=stdout, stderr=stderr)
                rec["pid"] = p.pid
                deadline = time.monotonic() + 180
                marker = re.compile(rf"^pass={READY_PASS} ", re.M)
                while time.monotonic() < deadline:
                    if p.poll() is not None:
                        rec["readiness"] = "process exited before ready marker"
                        break
                    stdout.flush()
                    if marker.search(stdout_path.read_text(errors="replace")):
                        rec["readiness"] = "pass marker observed while process alive"
                        break
                    time.sleep(0.1)
                else:
                    rec["readiness"] = "timeout waiting for ready marker"
                if rec["readiness"] != "pass marker observed while process alive":
                    p.kill()
                    rec["termination"] = "SIGKILL after invalid/incomplete liveness capture"
                    rec["returncode"] = p.wait()
                    results["records"].append(rec)
                    json.dump(results, open(OUT / "results.json", "w"), indent=2)
                    raise SystemExit(f"invalid thread readiness for {tag}: {rec['readiness']}")
                for i in range(SAMPLES):
                    if p.poll() is not None:
                        rec["samples"].append({"sample": i + 1, "error": "process exited before sample"})
                        break
                    ps = subprocess.run(["ps", "-M", "-p", str(p.pid)], text=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                    lines = ps.stdout.strip().splitlines()
                    thread_count = max(0, len(lines) - 1)
                    if p.poll() is not None or thread_count < 1:
                        rec["samples"].append({"sample": i + 1, "error": "process not live at ps sample",
                                                "ps_output": ps.stdout})
                        break
                    latest = [int(x) for x in re.findall(r"^pass=(\d+) ", stdout_path.read_text(errors="replace"), re.M)]
                    rec["samples"].append({"sample": i + 1, "timestamp": time.strftime('%Y-%m-%dT%H:%M:%S%z'),
                                            "threads": thread_count,
                                            "pass_seen": max(latest) if latest else None,
                                            "ps_output": ps.stdout})
                    if i + 1 < SAMPLES:
                        time.sleep(INTERVAL_S)
                p.kill()
                rec["termination"] = "SIGKILL after six live samples (intentional)"
                rec["returncode"] = p.wait()
                rec["active_stream_marker"] = "PC_COMPRESS_STREAM active=1" in stderr_path.read_text(errors="replace")
            if len(rec["samples"]) != SAMPLES or any("error" in x for x in rec["samples"]):
                rec["termination"] = "SIGKILL after incomplete liveness capture"
                results["records"].append(rec)
                json.dump(results, open(OUT / "results.json", "w"), indent=2)
                raise SystemExit(f"incomplete live thread samples for {tag}")
            results["records"].append(rec)
            json.dump(results, open(OUT / "results.json", "w"), indent=2)
            print(f"{tag}: samples={len(rec['samples'])} max_threads={max((s.get('threads', 0) for s in rec['samples']), default=0)} rc={rec['returncode']}", flush=True)
    print(f"wrote {OUT / 'results.json'}")


if __name__ == "__main__":
    main()
