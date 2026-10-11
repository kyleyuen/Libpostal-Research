#!/usr/bin/env python3
"""Build exact-output fuzz harnesses for release candidates 197 and 199 mimalloc-v2."""
import datetime as dt
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
RAW = ROOT / "research/experiments/199-token-span-storage/raw/fuzz-exact-candidate-build-v1"
OUT = ROOT / "research/experiments/199-token-span-storage/artifacts/fuzz-exact-candidates-v1"
SOURCE = ROOT / "research/tools/fuzz_pipeline_exact.c"
MIMALLOC = pathlib.Path("/opt/homebrew/opt/mimalloc/lib/mimalloc.o")
VARIANTS = {
    "candidate-197": {
        "archive": ROOT / "research/experiments/197-combined-query-removal/artifacts/release/libpostal.a",
        "extra": [],
    },
    "candidate-199-mimalloc-v2": {
        "archive": ROOT / "research/experiments/199-token-span-storage/artifacts/release/libpostal.a",
        "extra": [str(MIMALLOC), "-flto"],
    },
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
                "original_binary_reused": str(ROOT / "research/experiments/199-token-span-storage/artifacts/fuzz-exact-v1/original-000/fuzz_pipeline_exact"),
                "started_utc": utc(), "variants": []}
    for name, spec in VARIANTS.items():
        archive = spec["archive"]
        binary_dir = OUT / name
        binary_dir.mkdir()
        binary = binary_dir / "fuzz_pipeline_exact"
        argv = ["/usr/bin/clang", "-O2", "-g", "-I", str(ROOT / "libpostal/src"), *spec["extra"],
                str(SOURCE), str(archive), "-lm", "-o", str(binary)]
        rec = {"name": name, "archive": str(archive), "archive_sha256": sha(archive),
               "argv": argv, "start_utc": utc()}
        proc = subprocess.run([sys.executable, str(ROOT / "research/tools/run_locked.py"), "--", *argv],
                              cwd=ROOT, capture_output=True, text=True)
        rec.update({"end_utc": utc(), "exit": proc.returncode, "stdout": proc.stdout,
                    "stderr": proc.stderr, "binary_sha256": sha(binary) if binary.exists() else None})
        (RAW / (name + ".command.json")).write_text(json.dumps(rec, indent=2) + "\n")
        (RAW / (name + ".stdout")).write_text(proc.stdout)
        (RAW / (name + ".stderr")).write_text(proc.stderr)
        (RAW / (name + ".exit")).write_text(f"{proc.returncode}\n")
        manifest["variants"].append(rec)
        if proc.returncode:
            manifest["status"] = "fail"
            break
    manifest["ended_utc"] = utc()
    manifest["status"] = manifest.get("status", "pass")
    (RAW / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    return 0 if manifest["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
