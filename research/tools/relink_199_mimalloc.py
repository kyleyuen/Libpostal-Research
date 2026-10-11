#!/usr/bin/env python3
"""Relink immutable 199 O2/LTO artifacts with the accepted static allocator."""
import datetime as dt
import fcntl
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
EXP = ROOT / "research/experiments/199-token-span-storage"
SOURCE = ROOT / "research/experiments/003-batched-lookups/tools/dump_outputs.c"
ARCHIVE = EXP / "artifacts/release/libpostal.a"
MIMALLOC = pathlib.Path("/opt/homebrew/opt/mimalloc/lib/mimalloc.o")
LOCK = ROOT / "research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock"
OUT = EXP / "artifacts/release-mimalloc-v2"
RAW = EXP / "raw/mimalloc-relink-v2"


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda: f.read(1024 * 1024), b""):
            h.update(b)
    return h.hexdigest()


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def main():
    for p in (SOURCE, ARCHIVE, MIMALLOC):
        if not p.is_file():
            print(f"missing input: {p}", file=sys.stderr)
            return 2
    OUT.mkdir(parents=True, exist_ok=False)
    RAW.mkdir(parents=True, exist_ok=True)
    inputs = {str(p): sha(p) for p in (SOURCE, ARCHIVE, MIMALLOC)}
    manifest = {"schema_version": 1, "variant": "199-release-mimalloc-v2",
                "source_manifest_sha256": "eb83c36944062bb8db12add9ded4789b12d7f2b3f993463c3654cc6822b3c0fb",
                "build_profile": "candidate archive unchanged; O2/LTO dumper relink with canonical static mimalloc; scanner O0; no PGO",
                "inputs": inputs, "commands": [], "started_utc": utc()}
    lock_path = LOCK
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    with lock_path.open("a+b") as lock:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        for name in ("dump_outputs", "bench_ext", "fuzz_pipeline"):
            source = (SOURCE if name == "dump_outputs" else
                      ROOT / "research/tools" / (name + ".c"))
            argv = ["/usr/bin/clang", "-O2", "-g", "-flto", "-I", str(ROOT / "libpostal/src"),
                    str(MIMALLOC), str(source),
                    str(ARCHIVE), "-lm", "-o", str(OUT / name)]
            start = utc()
            p = subprocess.run(argv, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            end = utc()
            stem = RAW / name
            stem.with_suffix(".command.json").write_text(json.dumps({"argv": argv, "start_utc": start, "end_utc": end}, indent=2) + "\n")
            stem.with_suffix(".stdout").write_text(p.stdout)
            stem.with_suffix(".stderr").write_text(p.stderr)
            stem.with_suffix(".exit").write_text(f"{p.returncode}\n")
            row = {"name": name, "argv": argv, "start_utc": start, "end_utc": end,
                   "exit": p.returncode, "stdout": str(stem.with_suffix(".stdout")),
                   "stderr": str(stem.with_suffix(".stderr")),
                   "binary_sha256": sha(OUT / name) if (OUT / name).exists() else None}
            manifest["commands"].append(row)
            if p.returncode:
                manifest["status"] = "fail"
                break
        fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
    manifest["ended_utc"] = utc()
    manifest["status"] = manifest.get("status", "pass")
    manifest["artifacts"] = {p.name: sha(p) for p in OUT.iterdir() if p.is_file()}
    manifest["original_release_untouched"] = sha(ARCHIVE) == inputs[str(ARCHIVE)]
    (RAW / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    return 0 if manifest["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
