#!/usr/bin/env python3
"""Build the small, self-contained reviewer ZIP from an explicit path allowlist."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
ALLOWLIST = ROOT / "research/package/review-bundle-paths.txt"
MAX_MEMBER_BYTES = 50_000_000


def members() -> list[Path]:
    rows = [line.strip() for line in ALLOWLIST.read_text(encoding="utf-8").splitlines() if line.strip() and not line.startswith("#")]
    result: list[Path] = []
    errors: list[str] = []
    for rel in rows:
        p = Path(rel)
        candidate = (ROOT / p).resolve()
        if p.is_absolute() or ".." in p.parts or ROOT not in candidate.parents:
            errors.append(f"unsafe path: {rel}")
        elif not candidate.is_file():
            errors.append(f"missing path: {rel}")
        elif candidate.stat().st_size > MAX_MEMBER_BYTES:
            errors.append(f"member exceeds review-bundle limit ({MAX_MEMBER_BYTES}): {rel}")
        else:
            result.append(p)
    if errors:
        raise SystemExit("\n".join(errors))
    if len(result) != len(set(result)):
        raise SystemExit("duplicate entries in explicit allowlist")
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, help="ZIP path outside the workspace")
    parser.add_argument("--list", action="store_true", help="validate and print the allowlist without creating an archive")
    args = parser.parse_args()
    paths = members()
    if args.list:
        for p in paths:
            print(f"{(ROOT / p).stat().st_size}\t{p.as_posix()}")
        return 0
    if args.output is None:
        parser.error("--output is required unless --list is used")
    output = args.output.expanduser().resolve()
    if ROOT == output or ROOT in output.parents:
        raise SystemExit("write the reviewer ZIP outside the research workspace")
    if output.exists():
        raise SystemExit(f"refusing to overwrite: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    records = []
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for rel in paths:
            data = (ROOT / rel).read_bytes()
            records.append({"path": rel.as_posix(), "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
            archive.writestr(rel.as_posix(), data)
        manifest = {
            "schema_version": 1,
            "bundle_type": "compact-review-context",
            "source_workspace": str(ROOT),
            "files": records,
            "external_model": "not included; see SETUP.md for pinned local model manifest and retrieval notes",
        }
        archive.writestr("BUNDLE-MANIFEST.json", json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"output": str(output), "members": len(records), "zip_bytes": output.stat().st_size}, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
