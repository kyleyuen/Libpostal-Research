# Experiment 115 — re-measure of 114 (rt8) on a sixth unseen set (golden-v9-osm-unseen)

Date/time/timezone: created 2026-10-04T00:59:15-0700; run 2026-10-04T00:59:16-0700 → 2026-10-04T01:02:20-0700 (`raw/start.txt`, `raw/DONE`), America/Los_Angeles. Status: complete — **rt8 accepted (not slower)**

Set: `profiling/golden-v9-osm-unseen` (9,480 addresses, 55 cities absent from v3–v8, never used before this run). Procedure: 093/102 (`tools/run.sh`, `tools/analyze.py`). Outputs (`raw/output_check.txt`): psf and rt8 are identical to the original on all 9,480 addresses. 18 commands exited 0.
    | binary | unseen-v9 µs/address | speedup vs original (range) | vs psf (per-round range) | max peak footprint |
    |---|---:|---:|---:|---:|
    | orig | 18.367 | 1.000× (1.00–1.00) | 0.1893 (0.188–0.191) | 2.613 GB |
    | psf | 3.486 | 5.283× (5.24–5.31) | 1.0000 (1.000–1.000) | 2.543 GB |
    | rt8 | 3.469 | 5.281× (5.26–5.37) | 1.0070 (0.990–1.016) | 2.543 GB |
    rt8 vs psf per round: [0.9898, 1.011, 0.9977, 1.007, 1.0159] median 1.0070
rt8 vs psf: +0.7% median, per-round 0.990–1.016 (3/5 rounds faster), so it is not slower on unseen OSM data and is accepted under the unseen-data rule. As 114's stage timers predicted, it is about neutral on OSM-style addresses; its gain is on golden-v1-style data (+2.8%).
