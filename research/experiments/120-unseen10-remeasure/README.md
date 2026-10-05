# Experiment 120 — re-measure of 119 (cpi) on a seventh unseen set (golden-v10-osm-unseen)

Date/time/timezone: created 2026-10-04T01:34:13-0700; run 2026-10-04T01:34:13-0700 → 2026-10-04T01:37:27-0700, America/Los_Angeles. Status: complete — **cpi accepted**

Set: `profiling/golden-v10-osm-unseen` (8,567 addresses, 53 cities absent from v3–v9, never used before). Procedure: 093/102/115. Outputs: rt8 and cpi are identical to the original on all 8,567 addresses. 18 commands exited 0.
    | binary | unseen-v10 µs/address | speedup vs original (range) | vs rt8 (per-round range) | max peak footprint |
    |---|---:|---:|---:|---:|
    | orig | 17.499 | 1.000× (1.00–1.00) | 0.1841 (0.182–0.188) | 2.612 GB |
    | rt8 | 3.209 | 5.431× (5.32–5.50) | 1.0000 (1.000–1.000) | 2.538 GB |
    | cpi | 3.083 | 5.677× (5.58–5.73) | 1.0466 (1.015–1.066) | 2.538 GB |
    cpi vs rt8 per round: [1.0664, 1.0326, 1.0466, 1.0146, 1.0581] median 1.0466
cpi is faster than rt8 in every round (+1.5% to +6.6%, median +4.7%): 5.68× vs the original on unseen data.
