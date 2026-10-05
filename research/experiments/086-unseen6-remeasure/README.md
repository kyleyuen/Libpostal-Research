# Experiment 086 — re-measure of 085 rl8 on a third unseen set (golden-v6-osm-unseen)

Date/time/timezone: created 2026-10-03T04:42:34-0700; run 2026-10-03T04:42:34-0700 → 2026-10-03T04:45:45-0700, America/Los_Angeles. Status: complete — **rl8 rejected**

Dataset `profiling/golden-v6-osm-unseen` (9,450 addresses, 41 countries, 42% non-ASCII; never used before). Outputs (`raw/output_check.txt`): pcp and rl8 identical to the original on all 9,450 addresses. Speed (sequential, 5 rotating rounds, 20 passes; 15 runs exit 0; `raw/analyze.stdout`):
    | binary | unseen-v6 µs/address | speedup vs original (range) | vs previous row (per-round) | max peak footprint |
    |---|---:|---:|---:|---:|
    | orig | 19.499 | 1.000× (1.00–1.00) |  | 2.612 GB |
    | pcp | 3.850 | 5.059× (4.99–5.15) | 5.0594 (4.992–5.154) | 2.538 GB |
    | rl8 | 3.904 | 4.995× (4.94–5.04) | 0.9844 (0.978–0.989) | 2.538 GB |
rl8 is 1.6% slower than pcp in every round on unseen OSM-style data, matching its ≈0 / slightly negative OSM screen; its +4.7% is specific to the Senzing-derived sets. pcp: 5.06× on this third unseen set.
