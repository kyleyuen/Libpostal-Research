#!/usr/bin/env python3
"""stdin: osmium OPL; stdout: one {addr:* tags, osm_id} JSON object per line for every object tagged with both
addr:housenumber and addr:street (same selection as the Overpass query in fetch_osm.sh)."""
import json, re, sys
unesc = lambda s: re.sub(r"%([0-9a-fA-F]+)%", lambda m: chr(int(m.group(1), 16)), s)
for line in sys.stdin:
    f = line.rstrip("\n").split(" ")
    tf = next((x for x in f if x.startswith("T")), "T")[1:]
    if not tf: continue
    tags = dict(unesc(kv).split("=", 1) if "=" in kv else (unesc(kv), "") for kv in tf.split(","))
    if tags.get("addr:housenumber") and tags.get("addr:street"):
        t = {k: v for k, v in tags.items() if k.startswith("addr:")}; t["osm_id"] = f[0]
        print(json.dumps(t, ensure_ascii=False))
