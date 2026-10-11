#!/usr/bin/env python3
"""Read-only registry audit: dataset paths, reserved flags, counts, hashes."""
import hashlib, json, pathlib

root = pathlib.Path(__file__).resolve().parents[4]
cfg = json.loads((root / "research/suite/suite.json").read_text())
print("name\treserved\taddresses\ttxt\ttxt_sha256\ttsv\ttsv_sha256\tfrozen_reference")
for d in cfg["datasets"]:
    row = [d["name"], str(bool(d.get("reserved"))).lower()]
    for key in ("txt", "tsv", "frozen_reference"):
        p = root / d[key] if d.get(key) else None
        if p and p.is_file():
            data = p.read_bytes()
            val = hashlib.sha256(data).hexdigest()
            if key == "txt":
                row += [str(sum(bool(x.strip()) for x in data.splitlines())), d[key], val]
            elif key == "tsv": row += [d[key], val]
            else: row += [d[key]]
        elif key == "txt": row += ["MISSING", d.get(key, ""), ""]
        elif key == "tsv": row += [d.get(key, ""), "MISSING" if d.get(key) else ""]
        else: row += ["MISSING" if d.get(key) else ""]
    print("\t".join(row))
