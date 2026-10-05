# Experiment 075 — two-step prefetch for the vocabulary-slot loop (EXP_VOCAB_PF2)

Date/time/timezone: created 2026-10-03T02:30:20-0700, America/Los_Angeles. Status: **rejected (slower)**

Base: 074 pf2. Change (`sources/address_parser.c`, `sources/feature_dict.h` `fd_find_slot_h`): hash every eligible admin token once and prefetch its first slot; then read those slots and prefetch the arena entry on a tag match; the lookup loop then runs the unchanged probe with the stored hash (074's pattern applied to the slot loop's slot → arena chain). Output: golden-v1 sha 45574ee2… identical.
Screen (6 rounds; `raw/screen_analyze.stdout`):
    v1 pf2 [6.654, 6.627, 6.656, 6.633, 6.612, 6.622] median 6.63
    v1 pv2 [6.673, 6.631, 6.638, 6.672, 6.663, 6.681] median 6.668
    v1 pv2 speedup vs pf2 per round [0.9972, 0.9994, 1.0027, 0.9942, 0.9923, 0.9912] median 0.9957
    osm pf2 [4.245, 4.273, 4.274, 4.236, 4.284, 4.281] median 4.274
    osm pv2 [4.27, 4.305, 4.283, 4.289, 4.305, 4.286] median 4.287
    osm pv2 speedup vs pf2 per round [0.9941, 0.9926, 0.9979, 0.9876, 0.9951, 0.9988] median 0.9946
Decision: −0.4% / −0.5% → rejected; together with 074's pfv, the slot loop's misses are already overlapped by the core.
