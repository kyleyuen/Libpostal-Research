#!/usr/bin/env python3
"""Pick OpenAddresses files for the v15 set: per country, address layers smallest first until the
estimated address count reaches TARGET (100k plus a margin for records without street/number).
usage: select_sources.py oa-data.json oa-exact.json feasibility.json > oa-selection.json"""
import json, sys
TARGET, BYTES_PER_ADDR = 150000, 27   # 27 B/address: ae/az/drm-ar, 19,642 addresses in 534,336 bytes
# trial build (raw/trial-oa-manifest.json) kept fewer than 100k for these: many exact duplicate strings (kr) or
# records without a house number (au); select further files for them
TARGET_BY_CC = {"kr": 600000, "au": 400000}
RELABEL = {"us/pr/": "pr", "fr/974/": "re"}   # territories filed under another country's folder
data, exact, feas = (json.load(open(p)) for p in sys.argv[1:4])
want = {x["cc"] for x in feas}
cnt = {s: n for v in exact.values() for s, n in v["sources"]}
by = {}
for x in data:
    if x["layer"] != "addresses" or not x.get("job") or not x.get("size"): continue   # no size = no published output
    s = x["source"]; cc = next((c for p, c in RELABEL.items() if s.startswith(p)), s.split("/")[0])
    if cc in want: by.setdefault(cc, []).append(x)
out = []
for cc in sorted(by):
    tot = 0
    for x in sorted(by[cc], key=lambda x: (x["size"], x["source"])):
        if tot >= TARGET_BY_CC.get(cc, TARGET): break
        est = cnt.get(x["source"], x["size"] // BYTES_PER_ADDR); tot += est
        out.append({"cc": cc, "source": x["source"], "name": x["name"], "job": x["job"], "bytes": x["size"], "est_addresses": est,
                    "url": f"https://v2.openaddresses.io/batch-prod/job/{x['job']}/source.geojson.gz"})
json.dump(out, sys.stdout, indent=1)
print(f"{len(out)} files, {len({o['cc'] for o in out})} countries, {sum(o['bytes'] for o in out)/1e6:.0f} MB", file=sys.stderr)
