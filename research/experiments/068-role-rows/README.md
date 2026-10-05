# Experiment 068 — resolved role/const ids carry their weight row directly (on 067 `pl`)

Date/time/timezone: created 2026-10-02T22:47:49-0700, America/Los_Angeles. Status: **rejected (slower)**

Change (`EXP_ROLE_ROWS`, `crf.c`): role-id and constant-id entries store (row start, length) with bit 31 set when they fit (state: start < 2^26, length < 32; transition: start < 2^24, length < 128), so accumulation and row prefetch skip the row-index read; other ids unchanged.
Checks: outputs identical (golden-v1, novel, OSM, 2×2M fuzz; `raw/verify_summary.txt`).
Screen (6 rounds, `raw/screen_analyze.stdout`):
    v1 pl [6.871, 6.881, 6.806, 6.803, 6.85, 6.841] median 6.845
    v1 rr [6.893, 6.945, 6.877, 6.909, 6.845, 6.879] median 6.886
    v1 rr vs pl per round [0.9968, 0.9908, 0.9897, 0.9847, 1.0007, 0.9945]
    osm pl [4.396, 4.335, 4.373, 4.361, 4.3, 4.35] median 4.355
    osm rr [4.362, 4.374, 4.37, 4.382, 4.37, 4.402] median 4.372
    osm rr vs pl per round [1.0078, 0.9911, 1.0007, 0.9952, 0.984, 0.9882]
−0.6% golden-v1, −0.4% OSM: the extra decode branch in the hot loops costs more than the saved row-index reads.
