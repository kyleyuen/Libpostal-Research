# Experiment 067 — per-address phrase-string cache and length-aware phrase-feature appends (on 062 `lf`)

Date/time/timezone: created 2026-10-02T22:39:25-0700, America/Los_Angeles. Status: **accepted into the stack** (`pl`)

Change (`EXP_FA_LEN`, `address_parser.c`, `features.c`): (1) the joined address-dictionary and component phrase strings are built once per phrase per address into a buffer owned by that phrase index (generation counter reset at token 0; no state crosses addresses) instead of once per token of the phrase; (2) the phrase-type features ("phrase type+phrase", "unambiguous phrase type(+phrase)"), "phrase" and "commonly …" are appended through `feature_array_add_nbl`, which takes the value lengths from the cache instead of strlen (same bytes and strip rule). Note: in this experiment copy the call-site replacements are unconditional (the switch must be on).
Checks: `EXP_FA_LEN_VERIFY` asserts cached length == strlen for every cached phrase: 0 mismatches on golden-v1, novel, OSM and ≥8.39M phrases in 2M fuzz; outputs identical (golden-v1, novel, OSM, 2×2M fuzz) (`raw/verify_summary.txt`).
Screen (6 rounds, `raw/screen_analyze.stdout`):
    v1 lf [6.857, 6.884, 6.939, 6.868, 6.884, 6.855] median 6.876
    v1 pl [6.825, 6.871, 6.832, 6.862, 6.854, 6.82] median 6.843
    v1 pl vs lf per round [1.0047, 1.0019, 1.0157, 1.0009, 1.0044, 1.0051]
    osm lf [4.372, 4.349, 4.371, 4.353, 4.361, 4.347] median 4.357
    osm pl [4.298, 4.311, 4.343, 4.342, 4.325, 4.362] median 4.333
    osm pl vs lf per round [1.0172, 1.0088, 1.0064, 1.0025, 1.0083, 0.9966]
+0.5% golden-v1 (6/6), +0.6% OSM (5/6).
