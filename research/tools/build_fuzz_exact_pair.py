#!/usr/bin/env python3
"""Build the exact-output deterministic fuzz harness against two immutable archives."""
import datetime as dt
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
RAW = ROOT / "research/experiments/199-token-span-storage/raw/fuzz-exact-build-v1"
OUT = ROOT / "research/experiments/199-token-span-storage/artifacts/fuzz-exact-v1"
SOURCE = ROOT / "research/tools/fuzz_pipeline_exact.c"
VARIANTS = {
    "original-000": ROOT / "research/experiments/000-baseline/artifacts/libpostal.a",
    "candidate-199": ROOT / "research/experiments/199-token-span-storage/artifacts/release/libpostal.a",
}


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def sha(path):
    h = hashlib.sha256()
    with pathlib.Path(path).open("rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def main():
    RAW.mkdir(parents=True, exist_ok=False)
    OUT.mkdir(parents=True, exist_ok=False)
    manifest = {"schema_version": 1, "source": str(SOURCE), "source_sha256": sha(SOURCE),
                "started_utc": utc(), "variants": []}
    for name, archive in VARIANTS.items():
        dest = OUT / name
        dest.mkdir()
        binary = dest / "fuzz_pipeline_exact"
        argv = ["/usr/bin/clang", "-O2", "-g", "-I", str(ROOT / "libpostal/src"),
                str(SOURCE), str(archive), "-lm", "-o", str(binary)]
        record = {"name": name, "archive": str(archive), "archive_sha256": sha(archive),
                  "argv": argv, "start_utc": utc()}
        p = subprocess.run([sys.executable, str(ROOT / "research/tools/run_locked.py"), "--", *argv],
                           cwd=ROOT, capture_output=True, text=True)
        record.update({"end_utc": utc(), "exit": p.returncode,
                       "stdout": p.stdout, "stderr": p.stderr,
                       "binary_sha256": sha(binary) if binary.exists() else None})
        (RAW / (name + ".command.json")).write_text(json.dumps(record, indent=2) + "\n")
        (RAW / (name + ".stdout")).write_text(p.stdout)
        (RAW / (name + ".stderr")).write_text(p.stderr)
        (RAW / (name + ".exit")).write_text(f"{p.returncode}\n")
        manifest["variants"].append(record)
        if p.returncode:
            manifest["status"] = "fail"
            break
    manifest["ended_utc"] = utc()
    manifest["status"] = manifest.get("status", "pass")
    (RAW / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    return 0 if manifest["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
