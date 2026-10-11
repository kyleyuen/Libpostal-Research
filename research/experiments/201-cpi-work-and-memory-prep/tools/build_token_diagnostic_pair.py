#!/usr/bin/env python3
"""Build paired 201 diagnostic dumpers against the pinned CPI185 archive.

Run only under research/tools/run_locked.py after the AC/environment gate.
The sole mode difference is EXP_TOKEN_SLICES.
"""
from __future__ import annotations

import datetime as dt
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TREE = ROOT / "research/experiments/201-cpi-work-and-memory-prep/source-token-storage-v1"
HANDOFF = ROOT / "research/experiments/201-cpi-work-and-memory-prep/raw/token-storage-diagnostics-v2/handoff-v2.json"
HANDOFF_V1 = ROOT / "research/experiments/201-cpi-work-and-memory-prep/raw/token-storage-diagnostics-v1/handoff.json"
BASE = ROOT / "research/experiments/185-diagnostic-instrumentation/artifacts/build-20261007-1625-pattern-summary-fix/cpi/diagnostic/libpostal.a"
DUMPER_SOURCE = ROOT / "research/experiments/185-diagnostic-instrumentation/tools/dump_outputs.c"
OUT = ROOT / "research/experiments/201-cpi-work-and-memory-prep/artifacts/token-diagnostics-v4"
RAW = ROOT / "research/experiments/201-cpi-work-and-memory-prep/raw/token-storage-diagnostics-v2/build-pair-v3"
SRC = ROOT / "libpostal/src"
EXPECTED_BASE_SHA = "c0700a8b99e0f575fe98d8af913435c1d18e24d7896ef98e7e4f91dcc5684b56"
EXPECTED_SOURCE_MANIFEST_SHA = "d19c0e2ddf64791ca81cfcd217a03e7665520c5609ac95cad2bc05f88464d089"
TUS = ("address_parser", "tokens", "string_utils", "diag")


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def stamp() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def invoke(name: str, argv: list[str], cwd: Path, log_dir: Path) -> None:
    log_dir.mkdir(parents=True, exist_ok=False)
    command = {"argv": argv, "cwd": str(cwd), "start_utc": stamp()}
    (log_dir / "command.json").write_text(json.dumps(command, indent=2) + "\n")
    with (log_dir / "stdout").open("wb") as out, (log_dir / "stderr").open("wb") as err:
        p = subprocess.Popen(argv, cwd=cwd, stdout=out, stderr=err)
        code = p.wait()
    command["end_utc"] = stamp()
    command["exit"] = code
    (log_dir / "command.json").write_text(json.dumps(command, indent=2) + "\n")
    (log_dir / "exit").write_text(f"{code}\n")
    if code:
        raise RuntimeError(f"{name} failed with exit {code}; logs: {log_dir}")


def main() -> int:
    RAW.mkdir(parents=True, exist_ok=False)
    OUT.mkdir(parents=True, exist_ok=False)
    handoff = json.loads(HANDOFF.read_text())
    manifest_path = Path(handoff["source_manifest_path"])
    started = stamp()
    handoff_v1 = json.loads(HANDOFF_V1.read_text())
    checks = {
        "base_archive_sha256": sha(BASE),
        "source_manifest_sha256": sha(manifest_path),
        "source_tree_sha256": handoff_v1["source_tree_sha256"],
        "base_archive_expected": EXPECTED_BASE_SHA,
        "source_manifest_expected": EXPECTED_SOURCE_MANIFEST_SHA,
        "handoff_base_sha256": handoff["base_archive_sha256_verified"],
    }
    if checks["base_archive_sha256"] != EXPECTED_BASE_SHA:
        raise RuntimeError("CPI185 base archive hash mismatch: " + checks["base_archive_sha256"])
    if checks["source_manifest_sha256"] != EXPECTED_SOURCE_MANIFEST_SHA:
        raise RuntimeError("source manifest hash mismatch: " + checks["source_manifest_sha256"])
    if checks["handoff_base_sha256"] != EXPECTED_BASE_SHA:
        raise RuntimeError("handoff base hash mismatch")
    for line in manifest_path.read_text().splitlines():
        expected, rel = line.split(maxsplit=1)
        source_file = TREE / rel
        if sha(source_file) != expected:
            raise RuntimeError("frozen source file hash mismatch: " + rel)
    (RAW / "preflight.json").write_text(json.dumps({"time_utc": started, "checks": checks,
        "all_manifested_source_files_verified": True}, indent=2) + "\n")

    common = ["-std=gnu2x", "-DHAVE_CONFIG_H", "-Wall", "-Wextra",
              "-Wno-unused-function", "-Wno-unused-parameter", "-Wformat",
              "-Werror=format-security", "-Winit-self", "-Wno-sign-compare", "-g",
              "-D", "LIBPOSTAL_EXPORTS", "-O2"]
    flags_file = ROOT / "research/experiments/185-diagnostic-instrumentation/raw/build-20261007-1625-pattern-summary-fix/flags-cpi-diagnostic.txt"
    flag_lines = flags_file.read_text().splitlines()
    start_flags = flag_lines.index("VARIANT_FLAGS") + 1
    end_flags = flag_lines.index("119_DELIVERY_EXTRA_FLAGS")
    switches = flag_lines[start_flags:end_flags]
    if not switches or any(not token.startswith("-D") for token in switches):
        raise RuntimeError("Could not parse complete CPI185 variant flag block")
    common += switches + ["-DEXP185_DIAG", '-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"']
    includes = ["-I" + str(TREE), "-I" + str(TREE.parent), "-I" + str(SRC), "-I" + str(SRC.parent)]

    for mode, enabled in (("control", False), ("candidate", True)):
        mode_dir = OUT / mode
        obj_dir = mode_dir / "obj"
        mode_dir.mkdir()
        obj_dir.mkdir()
        archive = mode_dir / "libpostal.a"
        shutil.copyfile(BASE, archive)
        flags = list(common) + (["-DEXP_TOKEN_SLICES"] if enabled else [])
        objects: list[Path] = []
        for tu in TUS:
            source = TREE / (tu + ".c")
            obj = obj_dir / (tu + ".o")
            argv = ["/usr/bin/clang", *flags, *includes, "-c", str(source), "-o", str(obj)]
            invoke(f"{mode}-compile-{tu}", argv, ROOT, RAW / mode / ("compile-" + tu))
            objects.append(obj)
        invoke(mode + "-archive-replace", ["/usr/bin/ar", "-r", str(archive), *map(str, objects)], ROOT,
               RAW / mode / "archive-replace")
        invoke(mode + "-archive-index", ["/usr/bin/ranlib", str(archive)], ROOT,
               RAW / mode / "archive-index")
        dump = mode_dir / "dump_outputs"
        argv = ["/usr/bin/clang", "-O2", "-g", "-DEXP185_DIAG", *includes,
                str(DUMPER_SOURCE), str(archive), "-lm", "-o", str(dump)]
        invoke(mode + "-link-dumper", argv, ROOT, RAW / mode / "link-dumper")
        (mode_dir / "hashes.json").write_text(json.dumps({
            "mode": mode, "token_slices_enabled": enabled,
            "source_manifest_sha256": checks["source_manifest_sha256"],
            "base_archive_sha256": checks["base_archive_sha256"],
            "archive_sha256": sha(archive), "dump_outputs_sha256": sha(dump),
            "object_sha256": {obj.name: sha(obj) for obj in objects},
            "switches_sha256": sha(flags_file),
            "common_flags": common, "mode_delta": ["-DEXP_TOKEN_SLICES"] if enabled else [],
        }, indent=2) + "\n")
    report = {"status": "builds complete", "started_utc": started, "ended_utc": stamp(), "checks": checks,
              "modes": {mode: json.loads((OUT / mode / "hashes.json").read_text())
                        for mode in ("control", "candidate")},
              "parser_invoked": False}
    (RAW / "build-summary.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": report["status"], "control": report["modes"]["control"]["dump_outputs_sha256"],
                      "candidate": report["modes"]["candidate"]["dump_outputs_sha256"]}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except BaseException as e:
        print(repr(e), file=sys.stderr)
        raise
