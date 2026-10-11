#!/usr/bin/env python3
"""Build a private aggregate-diagnostic derivative from a candidate CRF source."""
import argparse
import datetime as dt
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
P185 = ROOT / "research/experiments/185-diagnostic-instrumentation"
BUILD185 = P185 / "artifacts/build-20261007-1625-pattern-summary-fix/prototype181/diagnostic"
CPI185 = P185 / "artifacts/build-20261007-1625-pattern-summary-fix/cpi/diagnostic"


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda: f.read(1024 * 1024), b""):
            h.update(b)
    return h.hexdigest()


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--artifact-dir", required=True)
    ap.add_argument("--raw-dir", required=True)
    ap.add_argument("--label", required=True)
    ap.add_argument("--translation-unit", action="append", required=True,
                    help="every candidate-modified translation unit to replace in the base archive")
    ap.add_argument("--base-archive")
    ap.add_argument("--variant", choices=("181", "cpi185"), required=True,
                    help="source/build pairing: 181 prototype or the matching 185 CPI diagnostic baseline")
    ap.add_argument("--extra-flag", action="append", default=[])
    args = ap.parse_args()
    source = pathlib.Path(args.source).resolve()
    artifact = pathlib.Path(args.artifact_dir).resolve()
    raw = pathlib.Path(args.raw_dir).resolve()
    expected_archive = (BUILD185 / "libpostal.a" if args.variant == "181"
                        else CPI185 / "libpostal.a").resolve()
    base_archive = pathlib.Path(args.base_archive).resolve() if args.base_archive else expected_archive
    if base_archive != expected_archive or not base_archive.is_file():
        ap.error(f"--variant {args.variant} requires matching base archive {expected_archive}")
    units = args.translation_unit
    if len(set(units)) != len(units) or any(not (source / unit).is_file() for unit in units):
        ap.error("translation-unit list must be unique and every source file must exist")
    libpostal_src = ROOT / "libpostal/src"
    compiler = "/usr/bin/clang"
    variant_flags = ["-DEXP_FUSED_FRONT", "-DEXP_EVENT_FLOW"]
    if args.variant == "cpi185":
        variant_flags = []
        lines = (P185 / "raw/build-20261007-1625-pattern-summary-fix/flags-cpi-diagnostic.txt").read_text().splitlines()
        in_variant = False
        for line in lines:
            if line == "VARIANT_FLAGS":
                in_variant = True
            elif line == "MODE_FLAGS":
                in_variant = False
            elif in_variant and line.startswith("-D"):
                variant_flags.append(line)
    known = ({"-DEXP_FUSED_FRONT", "-DEXP_EVENT_FLOW"} if args.variant == "181" else
             {"-DEXP_NO_RESET", "-DEXP_FEATURE_DICT", "-DEXP_REUSE", "-DEXP_VITERBI_NEON"})
    if not known.issubset(variant_flags):
        ap.error(f"variant flags do not match the expected {args.variant} build signature: missing {sorted(known - set(variant_flags))}")
    variant_flags.extend(args.extra_flag)
    artifact.mkdir(parents=True, exist_ok=False)
    raw.mkdir(parents=True, exist_ok=False)
    archive = artifact / "libpostal.a"
    exe = artifact / "dump_outputs"
    shutil.copy2(base_archive, archive)
    commands = []
    for unit in units:
        source_copy = source / unit
        object_name = source_copy.stem + ".o"
        obj = artifact / object_name
        commands.extend([
            [compiler, "-std=gnu2x", "-DHAVE_CONFIG_H", "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-unused-parameter", "-Wformat", "-Werror=format-security", "-Winit-self", "-Wno-sign-compare", "-g", "-DLIBPOSTAL_EXPORTS", "-O2", *variant_flags, "-DEXP185_DIAG", '-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"', "-I", str(source), "-I", str(source.parent), "-I", str(libpostal_src), "-I", str(libpostal_src.parent), "-c", str(source_copy), "-o", str(obj)],
            ["/usr/bin/ar", "-d", str(archive), object_name],
            ["/usr/bin/ar", "-rc", str(archive), str(obj)],
        ])
    commands.extend([
        ["/usr/bin/ranlib", str(archive)],
        [compiler, "-O2", "-g", "-DEXP185_DIAG", "-I", str(source), "-I", str(source.parent), "-I", str(libpostal_src), "-I", str(libpostal_src.parent), str(P185 / "tools/dump_outputs.c"), str(archive), "-lm", "-o", str(exe)],
    ])
    records = []
    for i, argv in enumerate(commands, 1):
        start = utc()
        locked_argv = [sys.executable, str(ROOT / "research/tools/run_locked.py"), "--", *argv]
        p = subprocess.run(locked_argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        end = utc()
        stem = raw / f"{i:02d}-{argv[1].split('/')[-1]}"
        stem.with_suffix(".command.json").write_text(json.dumps({"argv": argv, "start_utc": start, "end_utc": end}, indent=2) + "\n")
        stem.with_suffix(".stdout").write_text(p.stdout)
        stem.with_suffix(".stderr").write_text(p.stderr)
        stem.with_suffix(".exit").write_text(f"{p.returncode}\n")
        records.append({"argv": argv, "locked_argv": locked_argv, "start_utc": start, "end_utc": end, "exit_code": p.returncode})
        if p.returncode:
            break
    manifest = {"label": args.label, "variant": args.variant,
                "base_archive": str(base_archive), "translation_units": units,
                "source_sha256": {unit: sha(source / unit) for unit in units}, "archive_sha256": sha(archive) if archive.exists() else None,
                "dumper_sha256": sha(exe) if exe.exists() else None, "commands": records}
    (raw / "build-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    return next((r["exit_code"] for r in records if r["exit_code"]), 0)


if __name__ == "__main__":
    sys.exit(main())
