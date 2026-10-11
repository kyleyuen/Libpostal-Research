#!/usr/bin/env python3
"""Verify the 203 default-parser evidence copied into a normal repository clone.

This reads retained files only; it never loads the parser or starts workloads.
Run from any directory with: python3 research/tools/verify_203_clone_evidence.py
Add --require-tracked after the release commit to require every checked path in Git's index.
"""
from __future__ import annotations

import argparse
import hashlib
import filecmp
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROMOTED = Path("research/final/203-cpi-memory")
OUTPUT = Path("research/suite/results/203-stream-output-v1")
CLONE_MAP = OUTPUT / "clone-evidence/manifest.json"
TIMING_DIRS = [
    Path("research/suite/results/203-stream-gold-novel-v1"),
    Path("research/suite/results/203-stream-osm-v3-v13-v1"),
    Path("research/suite/results/203-stream-v14-v1"),
]
THREADS = Path("research/experiments/203-cpi-memory-regression/raw/stream-steady-thread-v1/thread-captures/results.json")
ORIGINAL_EXE = Path("research/experiments/182-benchmark-suite-prep/artifacts/original-000/bench_ext")
EXPECTED_ORIGINAL_EXE_SHA = "a4968702ab9415f3fc71f509d73284e5d228073c49c825aeb7798b5b6e061a50"
EXPECTED_CANDIDATE_EXE_SHA = "8bb0a15c7dad2ae21bb660189007b263be27567e61d4fa4ec18feff2c479fd04"


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def line_count(path: Path) -> int:
    with path.open("rb") as f:
        return sum(1 for _ in f)


def fail(msg: str) -> None:
    raise ValueError(msg)


def verify_sha_manifest(manifest: Path, base: Path, *, exclude_self: bool = False) -> list[Path]:
    checked: list[Path] = []
    for n, raw in enumerate(manifest.read_text().splitlines(), 1):
        if not raw.strip():
            continue
        try:
            expected, name = raw.split("  ", 1)
        except ValueError:
            fail(f"malformed checksum line {manifest}:{n}")
        rel = name.removeprefix("./")
        path = base / rel
        if not path.is_file():
            fail(f"missing checksum target: {path}")
        actual = digest(path)
        if actual != expected:
            fail(f"checksum mismatch: {path} expected={expected} actual={actual}")
        checked.append(path)
    if exclude_self and manifest in checked:
        fail(f"manifest unexpectedly includes itself: {manifest}")
    return checked


def parse_time_metric(stderr: str, label: str, path: Path) -> int:
    m = re.search(rf"^\s*(\d+)\s+{re.escape(label)}\s*$", stderr, re.MULTILINE)
    if not m:
        fail(f"missing `{label}` in {path}")
    return int(m.group(1))


def parse_parse_seconds(stdout: str, row_count: int, path: Path) -> float:
    vals = [float(x) for x in re.findall(r"^pass=\d+ us_per_addr=([0-9.]+)", stdout, re.MULTILINE)]
    if not vals:
        fail(f"no pass timing rows in {path}")
    return sum(v * row_count for v in vals) / 1_000_000.0


def repo_relative(raw: str | Path) -> Path:
    path = Path(raw)
    if path.is_absolute():
        parts = path.parts
        try:
            idx = parts.index("research")
        except ValueError:
            fail(f"historical absolute path is outside repository tree: {raw}")
        path = Path(*parts[idx:])
    if ".." in path.parts or path.is_absolute():
        fail(f"unsafe repository-relative path: {raw}")
    return path


def require_tracked(paths: list[Path]) -> None:
    # Git is used only for index membership; this does not alter the worktree.
    for raw in paths:
        p = repo_relative(raw)
        proc = subprocess.run(["git", "-C", str(ROOT), "ls-files", "--error-unmatch", "--", p.as_posix()],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if proc.returncode != 0:
            fail(f"not tracked by Git: {p}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--require-tracked", action="store_true", help="also require all checked paths in the Git index")
    args = ap.parse_args()
    checked_git: list[Path] = []

    package = ROOT / PROMOTED
    checksum = package / "SHA256SUMS"
    sidecar = package / "SHA256SUMS.sha256"
    # Validate sidecar then every copied source, artifact and documentation file.
    expected_sidecar = sidecar.read_text().split()[0]
    if digest(checksum) != expected_sidecar:
        fail("promoted package SHA256SUMS sidecar mismatch")
    package_files = verify_sha_manifest(checksum, package, exclude_self=True)
    source_files = verify_sha_manifest(package / "SOURCE_MANIFEST.sha256", package)
    if len(package_files) != 95 or len(source_files) != 16:
        fail(f"unexpected promoted manifest sizes: package={len(package_files)} sources={len(source_files)}")
    checked_git.extend([checksum, sidecar, package / "SOURCE_MANIFEST.sha256", *package_files])

    cand_exe = package / "artifacts/cpi-stream-203-v2/bench_ext"
    if digest(cand_exe) != EXPECTED_CANDIDATE_EXE_SHA:
        fail("promoted candidate executable hash mismatch")
    original_exe = ROOT / ORIGINAL_EXE
    if not original_exe.is_file() or digest(original_exe) != EXPECTED_ORIGINAL_EXE_SHA:
        fail(f"preserved original bench_ext missing or hash mismatch: {ORIGINAL_EXE}")
    checked_git.extend([ORIGINAL_EXE])

    suite = json.loads((ROOT / "research/suite/suite.json").read_text())
    datasets = {d["name"]: d for d in suite["datasets"]}
    mapping = json.loads((ROOT / CLONE_MAP).read_text())
    entries = mapping["datasets"]
    if len(datasets) != 14 or len(entries) != 14 or set(datasets) != {e["dataset"] for e in entries}:
        fail("suite dataset registry and clone-evidence map do not contain the same 14 datasets")
    output_doc = json.loads((ROOT / (OUTPUT / "correctness.json")).read_text())
    run_suite = json.loads((ROOT / (OUTPUT / "suite.json")).read_text())
    input_manifest = json.loads((ROOT / (OUTPUT / "manifest.json")).read_text())["datasets"]
    if len(output_doc["dumps"]) != 14 or not output_doc.get("negative_control", {}).get("detected", False):
        fail("correctness record is missing 14 dumps or negative-control detection")
    rows_total = 0
    candidate_paths: list[Path] = []
    original_paths: list[Path] = []
    for e in entries:
        name = e["dataset"]
        d = datasets[name]
        expected_rows = int(e["expected_rows"])
        if e["input_sha256"] != input_manifest[name]["txt"]["sha256"]:
            fail(f"clone map and suite manifest disagree on input hash for {name}")
        rows_total += expected_rows
        for key in ("txt", "tsv"):
            input_path = ROOT / d[key]
            if not input_path.is_file():
                fail(f"missing {name} {key}: {d[key]}")
            if digest(input_path) != input_manifest[name][key]["sha256"]:
                fail(f"input hash mismatch for {name} {key}")
            if line_count(input_path) != expected_rows:
                fail(f"input line count mismatch for {name} {key}")
            checked_git.append(Path(d[key]))
        original = ROOT / e["original_clone_path"]
        candidate = ROOT / e["candidate_path"]
        for label, path, meta in (("original", original, e["original"]), ("candidate", candidate, e["candidate"])):
            if not path.is_file():
                fail(f"missing {name} {label} JSONL: {path.relative_to(ROOT)}")
            if digest(path) != meta["sha256"] or path.stat().st_size != meta["bytes"]:
                fail(f"{name} {label} JSONL hash/size mismatch")
            if line_count(path) != expected_rows or int(meta["lines"]) != expected_rows:
                fail(f"{name} {label} JSONL row-count mismatch")
            checked_git.append(path.relative_to(ROOT))
        if not e.get("byte_identical") or not filecmp.cmp(original, candidate, shallow=False):
            fail(f"{name} original/candidate JSONL byte comparison failed")
        candidate_log = ROOT / OUTPUT / f"correctness/cpi-stream-203_{name}.err"
        if not candidate_log.is_file() or not re.search(r"PC_COMPRESS_STREAM active=1", candidate_log.read_text(errors="replace")):
            fail(f"candidate output stream marker missing for {name}")
        checked_git.append(candidate_log)
        result = output_doc["dumps"].get(f"cpi-stream-203/{name}", {})
        if (not result.get("identical") or int(result.get("lines", -1)) != expected_rows
                or result.get("candidate_sha256") != e["candidate"]["sha256"]
                or result.get("baseline_sha256") != e["original"]["sha256"]):
            fail(f"{name} original/candidate output record does not match clone evidence")
        original_paths.append(original)
        candidate_paths.append(candidate)
    if rows_total != 130341 or int(mapping.get("total_rows", -1)) != rows_total:
        fail(f"unexpected corpus row total: {rows_total}")
    checked_git.extend([CLONE_MAP, OUTPUT / "correctness.json", OUTPUT / "manifest.json", OUTPUT / "results.json", OUTPUT / "suite.json", OUTPUT / "report.md", OUTPUT / "invocation.json"])

    fuzz = output_doc.get("fuzz", {})
    baseline_variant = next(v for v in run_suite["variants"] if v["name"] == "original-000")
    for seed_num in (1, 2):
        seed = f"s{seed_num}"
        row = fuzz.get(f"cpi-stream-203/{seed}", {})
        if not row.get("identical") or row.get("inputs") != 2_000_000:
            fail(f"fuzz digest/status evidence incomplete for {seed}")
        candidate_fuzz = ROOT / OUTPUT / f"correctness/cpi-stream-203_fuzz_{seed}.txt"
        reference_fuzz = repo_relative(baseline_variant["fuzz_reference"][str(seed_num)])
        reference_path = ROOT / reference_fuzz
        if not candidate_fuzz.is_file() or not reference_path.is_file():
            fail(f"missing candidate or preserved original digest/status stream for {seed}")
        candidate_fuzz_log = ROOT / OUTPUT / f"correctness/cpi-stream-203_fuzz_{seed}.err"
        if (line_count(candidate_fuzz) != 2_000_000
                or not filecmp.cmp(candidate_fuzz, reference_path, shallow=False)):
            fail(f"candidate digest/status stream differs from frozen original reference for {seed}")
        if not candidate_fuzz_log.is_file() or not re.search(r"PC_COMPRESS_STREAM active=1", candidate_fuzz_log.read_text(errors="replace")):
            fail(f"candidate fuzz stream marker missing for {seed}")
        checked_git.extend([candidate_fuzz, candidate_fuzz_log, reference_fuzz])

    # Audit all natural-exit timing captures and their raw time -l maxima.
    timing_count = candidate_count = 0
    min_parse = float("inf")
    pairs: dict[tuple[str, int], dict[str, tuple[int, int]]] = {}
    timing_paths: list[Path] = []
    for run_dir in TIMING_DIRS:
        result_json = json.loads((ROOT / run_dir / "results.json").read_text())
        for exit_path in sorted((ROOT / run_dir / "timing").glob("*.exit")):
            name = exit_path.name.removesuffix(".exit")
            parts = name.rsplit("__", 2)
            if len(parts) != 3:
                fail(f"unexpected timing capture filename: {exit_path}")
            dataset, role, round_tag = parts
            round_num = int(round_tag.removeprefix("r"))
            role = "candidate" if role == "cpi-stream-203" else "original" if role == "original-000" else role
            if role not in {"candidate", "original"}:
                fail(f"unexpected timing role: {exit_path}")
            if exit_path.read_text().strip() != "0":
                fail(f"nonzero timing exit: {exit_path}")
            timing_count += 1
            stderr_path = exit_path.with_suffix(".stderr")
            stdout_path = exit_path.with_suffix(".stdout")
            if not stderr_path.is_file() or not stdout_path.is_file():
                fail(f"missing timing streams: {exit_path}")
            stderr = stderr_path.read_text(errors="replace")
            stdout = stdout_path.read_text(errors="replace")
            if role == "candidate":
                candidate_count += 1
                if not re.search(r"PC_COMPRESS_STREAM active=1", stderr):
                    fail(f"candidate stream marker missing: {stderr_path}")
            rows = int(next(e["expected_rows"] for e in entries if e["dataset"] == dataset))
            parse_s = parse_parse_seconds(stdout, rows, stdout_path)
            min_parse = min(min_parse, parse_s)
            if parse_s < 10.0:
                fail(f"parse-only duration below 10 seconds ({parse_s:.3f}s): {stdout_path}")
            rss = parse_time_metric(stderr, "maximum resident set size", stderr_path)
            footprint = parse_time_metric(stderr, "peak memory footprint", stderr_path)
            pairs.setdefault((dataset, round_num), {})[role] = (footprint, rss)
            timing_paths.extend([exit_path.relative_to(ROOT), stdout_path.relative_to(ROOT), stderr_path.relative_to(ROOT)])
            # Verify raw maxima agree with retained aggregate report for that role.
            summary = result_json["datasets"][dataset]["variants"]["cpi-stream-203" if role == "candidate" else "original-000"]
            # Aggregates are maxima across all rounds, so membership (not equality) is appropriate.
            if footprint > int(summary["peak_footprint_max"]) or rss > int(summary["peak_rss_max"]):
                fail(f"raw maxima exceed aggregate summary: {stderr_path}")
    if timing_count != 84 or candidate_count != 42 or len(pairs) != 42:
        fail(f"timing roster incomplete: captures={timing_count}, candidate={candidate_count}, pairs={len(pairs)}")
    for key, roles in pairs.items():
        if set(roles) != {"original", "candidate"}:
            fail(f"incomplete timing pair {key}")
        for metric_ix, metric in ((0, "peak footprint"), (1, "max RSS")):
            delta = roles["candidate"][metric_ix] - roles["original"][metric_ix]
            if delta >= 0:
                fail(f"nonnegative {metric} paired delta for {key}: {delta}")
    checked_git.extend(timing_paths)
    checked_git.extend(p.relative_to(ROOT) for d in TIMING_DIRS for p in [ROOT / d / "results.json", ROOT / d / "manifest.json", ROOT / d / "suite.json", ROOT / d / "invocation.json"])

    thread_doc = json.loads((ROOT / THREADS).read_text())
    records = thread_doc["records"]
    sample_total = sum(len(r["samples"]) for r in records)
    if len(records) != 6 or sample_total != 36:
        fail(f"thread capture count mismatch: captures={len(records)}, samples={sample_total}")
    for r in records:
        if r.get("gate_exit") != 0 or len(r["samples"]) != 6 or r.get("returncode") != -9:
            fail(f"invalid thread capture record: {r.get('tag')}")
        if r.get("role") == "cpi-stream-203" and not r.get("active_stream_marker"):
            fail(f"candidate thread record lacks active marker: {r.get('tag')}")
        for s in r["samples"]:
            if s.get("threads") != 1 or int(s.get("pass_seen", 0)) < 50:
                fail(f"invalid live thread sample in {r.get('tag')}")
    checked_git.append(THREADS)
    for r in records:
        for key in ("stdout", "stderr"):
            p = repo_relative(r[key])
            if not (ROOT / p).is_file():
                fail(f"missing retained thread log: {p}")
            checked_git.append(p)

    if args.require_tracked:
        # Include every retained raw timing/thread file checked above plus relevant inputs and manifests.
        require_tracked(sorted(set(checked_git), key=str))

    print("203 clone evidence verification: PASS")
    print(f"package files: {len(package_files)}; promoted source/header files: {len(source_files)}")
    print(f"inputs/output JSONLs: 14 datasets, {rows_total:,} rows; original/candidate byte-equal")
    print(f"timings: {timing_count} exit-0 captures ({candidate_count} active candidates), {len(pairs)} negative footprint/RSS pairs; minimum parse-only {min_parse:.3f}s")
    print(f"live threads: {len(records)} intentional post-sample terminations, {sample_total} samples, one thread each")
    if args.require_tracked:
        print(f"Git tracked-path check: PASS ({len(set(checked_git))} paths)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as exc:
        print(f"203 clone evidence verification: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
