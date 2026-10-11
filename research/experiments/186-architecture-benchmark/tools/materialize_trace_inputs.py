#!/usr/bin/env python3
"""Materialize a trace-safe edge-case subset as legacy one-address-per-line text."""

import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from edge_api_dump import select_safe_cases


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cases", required=True)
    ap.add_argument("--count", type=int, default=5)
    ap.add_argument("--output-txt", required=True)
    ap.add_argument("--output-meta", required=True)
    args = ap.parse_args()
    if args.count < 1:
        raise ValueError("count must be positive")

    registry = json.loads(Path(args.cases).read_text(encoding="utf-8"))
    if registry.get("schema_version") != 1:
        raise ValueError("unsupported edge-cases.json schema_version")
    cases = registry.get("cases")
    selected = []
    for position, (case, trace_safe) in enumerate(select_safe_cases(cases)):
        text = case.get("input_utf8", "")
        options = case["options"]
        if (trace_safe and text and "\n" not in text and "\r" not in text and
                options == {"language": None, "country": None}):
            selected.append((position, case, text))
            if len(selected) == args.count:
                break
    if len(selected) != args.count:
        raise ValueError(f"requested {args.count} trace inputs, found {len(selected)}")

    txt_path = Path(args.output_txt)
    meta_path = Path(args.output_meta)
    txt_path.parent.mkdir(parents=True, exist_ok=True)
    meta_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [text for _, _, text in selected]
    metadata = {
        "schema_version": 1,
        "source_registry": str(Path(args.cases)),
        "selection": "first manifest-order safe ordinary C strings with NULL/NULL options; empty/newline inputs excluded",
        "trace_options": {"language": None, "country": None},
        "records": [
            {"manifest_safe_position": pos, "case_id": case["id"],
             "category": case["category"], "text_line": i + 1,
             "input_utf8": text,
             "options": case["options"],
             "input_sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
             "input_bytes": len(text.encode("utf-8"))}
            for i, (pos, case, text) in enumerate(selected)
        ],
    }
    # Create both artifacts exclusively so an earlier trace input or mapping is preserved.
    with txt_path.open("x", encoding="utf-8", newline="\n") as out:
        out.write("\n".join(lines) + "\n")
    try:
        with meta_path.open("x", encoding="utf-8", newline="\n") as out:
            json.dump(metadata, out, ensure_ascii=True, indent=2)
            out.write("\n")
    except Exception:
        txt_path.unlink(missing_ok=True)
        raise
    print(f"materialized={len(lines)} trace-safe inputs", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
