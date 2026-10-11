# v14 source acquisition notes

Date: 2026-10-10 UTC. The retained raw JSON response modification times in
`provenance.json` provide per-file timestamps.

Commands used:

```text
python3 profiling/golden-v14-osm-unseen/build_unseen14.py fetch
python3 profiling/golden-v14-osm-unseen/build_unseen14.py build
```

The fetch loop uses `https://maps.mail.ru/osm/tools/overpass/api/interpreter`
and `https://overpass-api.de/api/interpreter`, alternates on retry, sends the
same user-agent, and waits four seconds between selected city requests. It
retries a failed request up to four attempts with increasing 20-second delays.
Observed first-endpoint 504 responses fell back to the second endpoint for
Fort Collins, Eugene, Gatineau, New Plymouth, Plock, Popayan, and Nakuru. The
other selected requests reported success on the first endpoint. These outcomes
are based on the captured command-session output; response bytes and final
element counts are independently pinned per city in `provenance.json`.

An initial 67-city acquisition was stopped after early 504s to fit the
validation window. A second broader attempt was also stopped after the first
three city responses. The final 15-city subset was selected for geographic
coverage and bounded acquisition time; it was not selected using address
counts, parser output, or candidate performance. Only responses for the final
selected cities are part of this frozen dataset. No parser, model, build, or
benchmark was run by the data-preparation worker.

The builder output was:

```text
rows=1801 countries=12 cities=13 overlap_removed=0 missing=0
unseen14.tsv SHA-256 c795d62b629390c7ddf01d416f0d030e0bd9f075cb33d514ce10792b5b05e977
unseen14.txt SHA-256 68e60e0da7e28258489f492c8682aed25c66a3054bbc56a29b5413711398d61b
```
