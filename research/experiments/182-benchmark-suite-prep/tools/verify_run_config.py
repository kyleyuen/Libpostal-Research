#!/usr/bin/env python3
"""Read-only preflight for the prepared config; does not read or load model files."""
import hashlib, json, pathlib

e = pathlib.Path(__file__).resolve().parents[1]
root = e.parents[2]
cfg_path = e / "run-suite-3way-181.json"
cfg = json.loads(cfg_path.read_text())
assert [v["name"] for v in cfg["variants"]] == ["original-000", "cpi", "release-candidate"]
assert cfg["baseline"] == "original-000" and cfg["settings"]["correctness_jobs"] == 1
expected_release = root / "research/experiments/181-release-180/artifacts/release"
assert pathlib.Path(cfg["variants"][2]["bench_ext"]).parent == expected_release
for v in cfg["variants"]:
    for key in ("bench_ext", "dump_outputs", "fuzz_pipeline"):
        p = pathlib.Path(v[key]); assert p.is_file(), f"missing {v['name']} {key}: {p}"
        print(f"binary {v['name']}/{key} bytes={p.stat().st_size} sha256={hashlib.sha256(p.read_bytes()).hexdigest()}")
assert len(cfg["datasets"]) == 13
total = 0
reserved = []
for d in cfg["datasets"]:
    txt = root / d["txt"]; tsv = root / d["tsv"]
    assert txt.is_file() and tsv.is_file(), f"missing data: {d['name']}"
    lines = txt.read_bytes().splitlines()
    rows = tsv.read_bytes().splitlines()
    assert len(lines) == len(rows) and all(b"\t" in x and x.split(b"\t", 1)[1] == y for x, y in zip(rows, lines)), f"TXT/TSV mismatch: {d['name']}"
    total += len(lines)
    if d.get("reserved"):
        reserved.append(d["name"])
    frozen = root / d["frozen_reference"] if d.get("frozen_reference") else None
    if frozen:
        assert frozen.is_file(), f"missing frozen reference: {d['name']}"
        assert len(frozen.read_bytes().splitlines()) == len(lines), f"frozen row count mismatch: {d['name']}"
    print(f"dataset {d['name']} rows={len(lines)} reserved={bool(d.get('reserved'))} txt={hashlib.sha256(txt.read_bytes()).hexdigest()} tsv={hashlib.sha256(tsv.read_bytes()).hexdigest()}")
assert total == 128540 and reserved == ["unseen-v12", "unseen-v13"]
print(f"PASS datasets=13 rows={total} reserved={','.join(reserved)} correctness_jobs=1 timing_serial=true model_loaded=false")
