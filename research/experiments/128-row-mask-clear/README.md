# Experiment 128 — re-zero only the score-matrix rows the previous address touched (EXP_ROW_CLEAR)

Date/time/timezone: created 2026-10-04T02:39:30-0700 (`raw/created.txt`); screen 2026-10-04T02:42:48-0700 → 2026-10-04T02:48:42-0700; America/Los_Angeles. Status: **rejected (no gain)**

Change (`raw/crf.diff`):
- **Load.** A 20-bit source-row mask per transition feature, from the model's weights.
- **Accumulation.** The per-token OR of the masks of the features added.
- **Clearing.** The next address re-zeroes exactly those rows. Invariant: all rows of tokens t ≥ 1 are zero except the recorded ones; growth or a non-direct path falls back to a full clear.
Unlike 066 there is no per-cell tracking; unlike 092 the tracking cost is one OR per feature.
Correctness: `rmv` (+ `EXP_ROW_CLEAR_VERIFY`: after every clear the whole matrix is checked for +0.0) and `rmc` are identical on golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2.
Screen vs cpi (`raw/screen_analyze2.stdout`):
        v1 median cpi 5.286 rmc 5.375; rmc speedup per round [0.9884, 0.9933, 0.9642, 1.0076, 0.9675, 0.9996] median 0.9909; rounds faster 1/6
        v1 min    cpi 5.16 rmc 5.245; rmc speedup per round [0.9947, 1.0017, 0.9798, 0.999, 0.9722, 0.9854] median 0.9901; rounds faster 1/6
        osm median cpi 3.402 rmc 3.369; rmc speedup per round [0.9552, 1.0188, 1.0041, 1.0139, 1.0086, 0.9988] median 1.0063; rounds faster 4/6
        osm min    cpi 3.331 rmc 3.306; rmc speedup per round [1.0012, 1.0233, 0.9991, 1.0012, 1.0055, 0.9969] median 1.0012; rounds faster 4/6
Decision: −0.9% on golden-v1, +0.6% (median) / +0.1% (min) on OSM. Rejected: with about 15 of 20 rows touched, many 160-byte clears plus the mask bookkeeping cost as much as the single large memset, which already runs at store bandwidth.
