# Experiment 066 — re-zero only the transition-score rows written by the previous address (on 062 `lf`)

Date/time/timezone: created 2026-10-02T20:36:30-0700, America/Los_Angeles. Status: **rejected (slower)**

Change (`EXP_ST_REZERO`, `crf.c`): state_trans (rows t ≥ 1) kept all-zero between addresses; the accumulation loop marks each written L-cell row in a per-token bitmask (row from a cls→row table); the next address's setup re-zeroes only the marked rows; full clear on growth or any doubt. Viterbi unchanged.
Checks: outputs identical (golden-v1, novel, OSM, 2×2M fuzz; `raw/verify_summary.txt`).
Screen (6 rounds, `raw/screen_analyze.stdout`):
    v1 lf [6.824, 6.813, 6.823, 6.803, 6.827, 6.868] median 6.824
    v1 rz [6.857, 6.853, 6.885, 6.873, 6.859, 6.863] median 6.861
    v1 rz vs lf per round [0.9952, 0.9942, 0.991, 0.9898, 0.9953, 1.0007]
    osm lf [4.309, 4.328, 4.349, 4.36, 4.301, 4.324] median 4.326
    osm rz [4.377, 4.39, 4.382, 4.435, 4.376, 4.393] median 4.386
    osm rz vs lf per round [0.9845, 0.9859, 0.9925, 0.9831, 0.9829, 0.9843]
−0.5% golden-v1, −1.4% OSM: transition features touch most rows, so the marking costs more than the skipped clearing.
