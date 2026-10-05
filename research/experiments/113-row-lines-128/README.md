# Experiment 113 — re-test of 085's multi-line weight-row prefetch on the cleaned machine, with 128-byte steps (EXP_ROW_LINES / EXP_ROW_STEP)

Date/time/timezone: created 2026-10-04T00:30:20-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: **rejected (OSM slower, as in 085)**

Why re-test a rejected idea:
- **Attribution.** 112's cold-minus-warm line profile puts the largest single first-touch cost in weight accumulation (crf.c:674 state rows ≈0.29, :688 transition rows ≈0.20 µs/address, profiler-inflated scale).
- **Changed conditions.** 085/086 were measured while two orphaned processes loaded the shared cache (removed before 106/107).
- **Defect in 085.** It stepped 64 bytes, while the Apple Silicon line is 128 bytes (`hw.cachelinesize`), so half its prefetches were redundant.

Change (`raw/crf.diff`, hints only): every line of each looked-up state/transition weight row is prefetched, up to 8 prefetches per row; `r64` = 085's stepping, `r128` = 128-byte stepping. golden-v1 outputs identical (sha 45574ee2…).
Screen vs psf (6 rounds; `raw/screen_analyze2.stdout`; rounds noisy, load average ≈2.5):
        v1 median psf 5.812 r64 5.686; r64 speedup per round [1.0279, 1.1849, 1.0516, 0.9513, 0.9163, 1.194] median 1.0398; rounds faster 4/6
        v1 min    psf 5.649 r64 5.532; r64 speedup per round [1.0318, 1.1512, 1.0352, 0.9889, 0.9378, 1.132] median 1.0335; rounds faster 4/6
        osm median psf 3.617 r64 3.804; r64 speedup per round [0.9777, 0.7946, 0.881, 0.9775, 0.9751, 0.9883] median 0.9763; rounds faster 0/6
        osm min    psf 3.561 r64 3.737; r64 speedup per round [0.9647, 0.8558, 0.9063, 0.9815, 0.9646, 0.9967] median 0.9647; rounds faster 0/6
        v1 median psf 5.812 r128 5.466; r128 speedup per round [1.0326, 1.0742, 1.0583, 1.0324, 1.0772, 1.2158] median 1.0663; rounds faster 6/6
        v1 min    psf 5.649 r128 5.388; r128 speedup per round [1.0219, 1.0651, 1.048, 1.0195, 1.0675, 1.1477] median 1.0566; rounds faster 6/6
        osm median psf 3.617 r128 3.784; r128 speedup per round [0.9763, 0.9189, 0.9583, 0.9815, 0.9852, 0.9994] median 0.9789; rounds faster 0/6
        osm min    psf 3.561 r128 3.675; r128 speedup per round [0.9647, 0.9354, 0.9446, 0.9809, 0.9809, 0.9997] median 0.9728; rounds faster 0/6
Decision: the result replicates 085 on the clean machine.
- **r128:** +6.6% on golden-v1 (6/6 rounds) and −2.1% (median) / −2.7% (min) on OSM (0/6 rounds faster).
- **r64:** +4% / −2.4%.
- **Why:** OSM-style addresses mostly reuse cached weight rows, and the prefetch issue cost (085: ≈0.2 µs) is not recovered. A Senzing-specific gain fails the generality rule, so this is rejected without spending an unseen set.
