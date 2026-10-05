# Experiment 102 — re-measure of 098 (npr) and 101 (psf) on a fifth unseen set (golden-v8-osm-unseen)

Date/time/timezone: created per `raw/created.txt`; run 2026-10-03T22:36:14-0700 → 2026-10-03T22:40:40-0700 (`raw/start.txt`, `raw/DONE`), America/Los_Angeles. Status: complete — **npr and psf accepted**

Set: `profiling/golden-v8-osm-unseen` (9,981 addresses, 55 cities absent from v3–v7, never used before this run). Script `tools/run.sh` (093's procedure); analysis `tools/analyze.py`. The golden-v9 Overpass fetch ran concurrently (network only).
Outputs (`raw/output_check.txt`): vmx, npr and psf are identical to the original on all 9,981 addresses. Speed (sequential, 5 rotating rounds, 20 passes; 24 commands exit 0; `raw/analyze.stdout`):

| binary | unseen-v8 µs/address | speedup vs original (range) | vs vmx (per-round range) | max peak footprint |
|---|---:|---:|---:|---:|
| orig | 19.574 | 1.000× (1.00–1.00) | 0.1948 (0.188–0.196) | 2.612 GB |
| vmx | 3.787 | 5.133× (5.11–5.32) | 1.0000 (1.000–1.000) | 2.543 GB |
| npr | 3.747 | 5.226× (5.21–5.33) | 1.0149 (1.002–1.022) | 2.543 GB |
| psf | 3.633 | 5.369× (5.35–5.51) | 1.0461 (1.036–1.046) | 2.543 GB |

    npr vs vmx per round: [1.0061, 1.0149, 1.0171, 1.0219, 1.0016] median 1.0149
    psf vs npr per round: [1.04, 1.031, 1.0284, 1.0238, 1.0347] median 1.0310
    psf vs vmx per round: [1.0464, 1.0463, 1.046, 1.0461, 1.0363] median 1.0461
npr is faster than vmx in every round (+1.5%); psf is faster than npr in every round (+3.1%). Both increments hold on unseen data, so 098 and 101 are accepted. Next: validation of psf (103).
Machine note: unrelated long-running processes were using CPU throughout (`raw/top_before.txt`): two orphaned VS Code crashpad handlers at ≈170% each (up for ≈30 days, so present in earlier validations too), `icdd` at ≈99% and a temp process at ≈80%.
