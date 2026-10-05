# Experiment 087 — weight-row density (measurement for a dense/vectorized accumulation idea)

Date/time/timezone: created 2026-10-03T04:46:40-0700, America/Los_Angeles. Status: complete — **idea not viable**

Question: are the transition-feature rows used per address dense enough (close to L×L = 400 cells) that storing them densely (8 B/cell, NEON adds) would be smaller and faster than the packed sparse records (10 B/cell)?
Method: `EXP_ROW_STATS` counter build on pcp sources (-O2, `raw/rst_*.stderr`).
    ROW_STATS model: state rows=11470623 cells=33799350; trans rows=2086474 cells=13369573 mean=6.4 rows>=320 cells=0
    development: ROW_STATS used per address (n=10000): state keys 38.55 cells 477.7 | trans keys 8.33 cells 614.7 (73.8 per key), keys >=320 cells 0.00, >=200 0.00 | hist/57: 44345 17044 14544 7338 0 0 0 0
    unseen v6:   ROW_STATS used per address (n=9000): state keys 34.56 cells 436.0 | trans keys 7.59 cells 582.4 (76.7 per key), keys >=320 cells 0.00, >=200 0.00 | hist/57: 36275 12597 13064 6401 0 0 0 0
Conclusion: used transition rows average 74–77 of 400 cells (none ≥ 200 in use; no model row ≥ 320), so a dense form would be ≈4× larger per row; accumulation is ≈1,000 scattered adds per address (≈2 cycles each warm). Not pursued.
