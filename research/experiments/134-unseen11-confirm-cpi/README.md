# Experiment 134 — confirmation of the deliverable cpi on golden-v11 (validation only)

Date/time/timezone: created 2026-10-04T03:14:14-0700; run 2026-10-04T03:14:15-0700 → 2026-10-04T03:16:18-0700 (`raw/start.txt`, `raw/DONE`), America/Los_Angeles. Status: complete

cpi was already accepted (120) and validated (121); this run selects nothing. Set: `profiling/golden-v11-osm-unseen` (7,882 addresses, 51 cities absent from v3–v10, never used before). Procedure: 093/120 with the original and cpi only (the comment lines of `tools/run.sh` were corrected after the run; the commands are unchanged). The golden-v12 fetch ran concurrently (network only). 12 commands exited 0.
Outputs: cpi identical to the original on all 7,882 addresses (`raw/output_check.txt`).
    | binary | unseen-v11 µs/address | speedup vs original (range) | vs cpi (per-round range) | max peak footprint |
    |---|---:|---:|---:|---:|
    | orig | 17.848 | 1.000× (1.00–1.00) | 0.1774 (0.174–0.179) | 2.612 GB |
    | cpi | 3.167 | 5.637× (5.60–5.76) | 1.0000 (1.000–1.000) | 2.542 GB |
cpi is **5.64×** faster than the original on a second set it has never seen (v10: 5.68×).
