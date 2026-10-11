#!/usr/bin/env python3
"""Link API-test shared libraries from the immutable 197/199 static archives."""
import datetime as dt
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
MIMALLOC = pathlib.Path("/opt/homebrew/opt/mimalloc/lib/mimalloc.o")
OUTS = {
    "197": (ROOT / "research/experiments/197-combined-query-removal/artifacts/release/libpostal.a",
            ROOT / "research/experiments/197-combined-query-removal/artifacts/api-edge-v1/libpostal.dylib", []),
    "199-mimalloc-v2": (ROOT / "research/experiments/199-token-span-storage/artifacts/release/libpostal.a",
                        ROOT / "research/experiments/199-token-span-storage/artifacts/api-mimalloc-v2/libpostal.dylib",
                        [str(MIMALLOC), "-flto"]),
}
RAW = ROOT / "research/analysis/parallel-exploration-2026-10-07/raw/api-edge-link-v1"


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
    manifest = {"schema_version": 1, "kind": "api_edge_dynamic_links", "started_utc": utc(), "variants": []}
    for name, (archive, output, extra) in OUTS.items():
        output.parent.mkdir(parents=True, exist_ok=False)
        argv = ["/usr/bin/clang", "-dynamiclib", "-Wl,-all_load", *extra, str(archive), "-lm", "-o", str(output)]
        rec = {"name": name, "archive": str(archive), "archive_sha256": sha(archive), "argv": argv, "start_utc": utc()}
        if extra:
            rec["extra_inputs_sha256"] = {str(MIMALLOC): sha(MIMALLOC)}
        proc = subprocess.run([sys.executable, str(ROOT / "research/tools/run_locked.py"), "--", *argv],
                              cwd=ROOT, capture_output=True, text=True)
        rec.update({"end_utc": utc(), "exit": proc.returncode, "stdout": proc.stdout,
                    "stderr": proc.stderr, "library_sha256": sha(output) if output.exists() else None})
        stem = RAW / name
        stem.with_suffix(".command.json").write_text(json.dumps(rec, indent=2) + "\n")
        stem.with_suffix(".stdout").write_text(proc.stdout)
        stem.with_suffix(".stderr").write_text(proc.stderr)
        stem.with_suffix(".exit").write_text(f"{proc.returncode}\n")
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
