#!/usr/bin/env python3
"""usage: osm2jsonl.py OVERPASS.json OUT.jsonl -- one {addr:* tags, osm_id} object per line for build_dataset.py.
Fails (exit 1) on Overpass runtime errors or an empty result, so a timed-out query is never taken as data."""
import json, sys
d = json.load(open(sys.argv[1], encoding="utf-8"))
if d.get("remark") and "error" in d["remark"].lower(): sys.exit(f"overpass: {d['remark']}")
els = d.get("elements", [])
if not els: sys.exit("no elements")
with open(sys.argv[2], "w", encoding="utf-8") as o:
    for e in els:
        t = {k: v for k, v in e.get("tags", {}).items() if k.startswith("addr:")}
        t["osm_id"] = f"{e['type'][0]}{e['id']}"
        o.write(json.dumps(t, ensure_ascii=False) + "\n")
