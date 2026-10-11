#!/usr/bin/env python3
"""Create auditable Git/release path lists from the current workspace tree.

This only reads Git's untracked/ignored inventories and file metadata. It does
not hash, copy, compress, stage, or modify research artifacts.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "research" / "package"
SMALL_RAW_SUFFIXES = {
    ".json", ".yaml", ".yml", ".toml", ".command", ".stderr", ".stdout",
    ".log", ".sha256", ".sha256s", ".patch", ".diff", ".exit", ".footprint",
}
SOURCE_SUFFIXES = {".py", ".c", ".h", ".sh"}
DOC_NAMES = {"README.md", "REPORT.md", "METRICS.md", "METHOD.md", "SETUP.md", "REVIEW_GUIDE.md"}
MAX_GIT_EVIDENCE_FILE = 1_000_000
MAX_GIT_SOURCE_FILE = 5_000_000
LOCAL_EXCLUDES = (
    "/.git/", "/__pycache__/", "/.pytest_cache/", "/.venv/", "/venv/",
)
LOCAL_SUFFIXES = {".o", ".pyc", ".profraw", ".profdata", ".lock"}


def git_paths(*args: str) -> list[str]:
    raw = subprocess.check_output(["git", *args, "-z"], cwd=ROOT)
    return sorted({p for p in raw.decode("utf-8", "surrogateescape").split("\0") if p})


def expand_dirs(paths: list[str]) -> list[str]:
    expanded: set[str] = set()
    for rel in paths:
        p = ROOT / rel
        if p.is_dir() and not p.is_symlink():
            for base, dirs, files in os.walk(p):
                dirs[:] = [d for d in dirs if d != ".git"]
                for name in files:
                    expanded.add((Path(base) / name).relative_to(ROOT).as_posix())
        else:
            expanded.add(rel.rstrip("/"))
    return sorted(expanded)


def size_of(rel: str) -> int:
    try:
        return (ROOT / rel).lstat().st_size
    except OSError:
        return 0


def local_noise(rel: str) -> bool:
    normalized = "/" + rel.replace("\\", "/") + "/"
    p = Path(rel)
    # These are original-library output references used for 203 byte checks,
    # plus the pinned historical fuzz streams used as correctness evidence.
    # Keep them in the release asset even though other disposable caches may
    # eventually be added to the local-noise rules.
    if rel.startswith("research/suite/results/cache/dump_") and rel.endswith(".jsonl"):
        return False
    if rel in {
        "research/experiments/011-normalize-fastpaths/raw/fuzz/base_s1.txt",
        "research/experiments/011-normalize-fastpaths/raw/fuzz/base_s2.txt",
    }:
        return False
    return (
        any(token in normalized for token in LOCAL_EXCLUDES)
        or p.name == ".DS_Store"
        or p.suffix.lower() in LOCAL_SUFFIXES
        or p.name.endswith(".dSYM")
        or "/libpostal/" in normalized
    )


def git_candidate(rel: str, size: int) -> bool:
    p = Path(rel)
    parts = p.parts
    if "__pycache__" in parts or p.suffix.lower() in {".pyc", ".profraw", ".profdata", ".lock"}:
        return False
    if rel in {"SETUP.md", "REVIEW_GUIDE.md"}:
        return True
    if rel.startswith("research/package/"):
        return True
    # Clone-first 203 review is self-contained in Git: promoted source/build
    # manifests, exact output streams, the copied original JSONL references,
    # and the complete timing/steady/thread evidence. All these files are
    # individually below GitHub's 100 MB limit.
    if rel.startswith("research/final/203-cpi-memory/"):
        return size <= 50_000_000
    if rel.startswith("research/suite/results/203-stream-output-v1/"):
        return size <= 50_000_000
    if rel.startswith((
        "research/suite/results/203-stream-gold-novel-v1/",
        "research/suite/results/203-stream-osm-v3-v13-v1/",
        "research/suite/results/203-stream-v14-v1/",
    )):
        return size <= 50_000_000
    if rel.startswith("research/experiments/203-cpi-memory-regression/raw/"):
        return size <= 50_000_000
    if rel.startswith("research/experiments/000-baseline/artifacts/"):
        return size <= 50_000_000
    if rel.startswith("research/experiments/182-benchmark-suite-prep/artifacts/original-000/"):
        return ".dSYM" not in p.parts and size <= 50_000_000
    if rel.startswith("profiling/golden-v14-osm-unseen/") or rel.startswith("profiling/independent-audit-v1/"):
        return size <= 50_000_000
    if len(parts) >= 3 and parts[0:2] == ("research", "experiments"):
        if p.name in DOC_NAMES:
            return True
        if not (parts[2][:3].isdigit() and 150 <= int(parts[2][:3]) <= 203):
            return False
        # Keep authored tools/tests and source deltas in Git. Full immutable
        # source snapshots and generated tables are carried by release assets.
        if "tools" in parts and p.suffix.lower() in SOURCE_SUFFIXES and size <= MAX_GIT_SOURCE_FILE:
            return True
        if p.suffix.lower() in {".patch", ".diff", ".sha256", ".sha256s"} and size <= 5_000_000:
            return True
        if "raw" in parts and p.suffix.lower() in SMALL_RAW_SUFFIXES and size <= MAX_GIT_EVIDENCE_FILE:
            return True
        return False
    if rel.startswith("research/suite/results/"):
        if rel.startswith("research/suite/results/cache/dump_578a62678e98_") and rel.endswith(".jsonl"):
            return False  # clone-evidence contains the versioned copy with provenance
        return p.suffix.lower() in SMALL_RAW_SUFFIXES and size <= MAX_GIT_EVIDENCE_FILE
    return False


def main() -> int:
    PACKAGE.mkdir(parents=True, exist_ok=True)
    ordinary = expand_dirs(git_paths("ls-files", "--others", "--exclude-standard"))
    ignored = expand_dirs(git_paths("ls-files", "--others", "--ignored", "--exclude-standard"))
    tracked = set(git_paths("ls-files", "--cached"))
    ordinary = [p for p in ordinary if p not in tracked]
    ignored = [p for p in ignored if p not in tracked and p not in set(ordinary)]

    git_items: list[tuple[str, int, str]] = []
    release_items: list[tuple[str, int, str]] = []
    local_items: list[tuple[str, int, str]] = []
    for rel in ordinary:
        n = size_of(rel)
        if git_candidate(rel, n):
            git_items.append((rel, n, "untracked-portable"))
        elif local_noise(rel):
            local_items.append((rel, n, "local-cache-or-noise"))
        else:
            release_items.append((rel, n, "untracked-evidence-or-source"))
    for rel in ignored:
        n = size_of(rel)
        # A narrow explicit allowlist for the portable 203 clone review and
        # pinned original artifacts takes precedence over broad ignore rules.
        if git_candidate(rel, n):
            git_items.append((rel, n, "ignored-portable-review-evidence"))
        elif local_noise(rel):
            local_items.append((rel, n, "ignored-cache-or-reproducible-intermediate"))
        else:
            release_items.append((rel, n, "ignored-binary-or-raw-evidence"))

    def write_list(name: str, items: list[tuple[str, int, str]]) -> None:
        (PACKAGE / name).write_text("".join(f"{p}\t{n}\t{tag}\n" for p, n, tag in sorted(items)), encoding="utf-8")

    write_list("git-add-candidates.tsv", git_items)
    write_list("release-paths.tsv", release_items)
    write_list("local-exclusions.tsv", local_items)
    summary = {
        "schema_version": 1,
        "generated_from": "git ls-files --others/--ignored plus current lstat sizes",
        "hashing_performed": False,
        "staging_or_archiving_performed": False,
        "tracked_paths_not_relisted": len(tracked),
        "git_add_candidates": {"files": len(git_items), "bytes": sum(n for _, n, _ in git_items)},
        "release_paths": {"files": len(release_items), "bytes": sum(n for _, n, _ in release_items)},
        "local_exclusions": {"files": len(local_items), "bytes": sum(n for _, n, _ in local_items)},
        "notes": [
            "The Git list is a proposal, not an executed git add.",
            "Release paths are content to retain outside ordinary Git; split/compress only after freeze.",
            "The nested upstream libpostal checkout and external model cache are excluded and require pinned retrieval.",
        ],
    }
    (PACKAGE / "inventory-summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
