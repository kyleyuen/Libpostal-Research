# Experiment 097 — four-at-a-time weight accumulation within a row (EXP_ACC_UNROLL)

Date/time/timezone: created 2026-10-03T11:35:06-0700, America/Los_Angeles. Status: **rejected (no gain)**

Base vmx. Change (`sources/crf.c`): when every row of both packed weight matrices has pairwise-distinct classes (checked once at load; a first version required strictly increasing classes, which the model's rows are not, so its path never ran — preserved in `raw/attempt1_increasing_check/`), each row's additions are issued four at a time (four record loads, four cell loads, four stores); per-cell sums and their order are unchanged. Load check result: distinct = 1. Outputs: identical on golden-v1, novel, OSM, unseen v4, frozen references, fuzz seeds 1 and 2 (`raw/verify_acu/summary.txt`).
Screen vs vmx (6 rounds; `raw/screen_analyze.stdout`):
    v1 vmx [6.373, 6.367, 6.376, 6.38, 6.347, 6.351] median 6.37
    v1 acu [6.405, 6.38, 6.39, 6.441, 6.411, 6.387] median 6.397
    v1 acu speedup vs vmx per round [0.995, 0.998, 0.9978, 0.9905, 0.99, 0.9944] median 0.9947
    osm vmx [4.066, 4.066, 4.079, 4.049, 4.048, 4.113] median 4.066
    osm acu [4.093, 4.061, 4.062, 4.06, 4.103, 4.063] median 4.062
    osm acu speedup vs vmx per round [0.9934, 1.0012, 1.0042, 0.9973, 0.9866, 1.0123] median 0.9993
−0.5% golden-v1, ≈0 OSM → rejected; the core already overlaps these read-add-writes.
