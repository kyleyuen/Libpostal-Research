# Experiment 081 — prefetch at key production for integer feature keys (EXP_PUSH_PREFETCH[2])

Date/time/timezone: created 2026-10-03T03:14:01-0700, America/Los_Angeles. Status: **rejected (no gain)**

Base pcp. Change (`sources/crf.c` `crf_ik_push`, hints only): when a key already resolved to a feature id is produced during feature generation, prefetch its row-index entry (pp1); pp2 also prefetches the integer-table slot of unresolved integer keys. golden-v1 sha identical for both.
Screen (6 rounds; `raw/screen_analyze.stdout`):
    v1 pcp [6.48, 6.482, 6.505, 6.489, 6.5, 6.487] median 6.488
    v1 pp1 [6.507, 6.493, 6.49, 6.492, 6.5, 6.494] median 6.494
    v1 pp2 [6.494, 6.507, 6.512, 6.489, 6.506, 6.507] median 6.506
    v1 pp1 speedup vs pcp per round [0.9959, 0.9983, 1.0023, 0.9995, 1.0, 0.9989] median 0.9992
    v1 pp2 speedup vs pcp per round [0.9978, 0.9962, 0.9989, 1.0, 0.9991, 0.9969] median 0.9984
    osm pcp [4.158, 4.163, 4.193, 4.157, 4.156, 4.173] median 4.161
    osm pp1 [4.204, 4.162, 4.149, 4.164, 4.194, 4.173] median 4.168
    osm pp2 [4.19, 4.204, 4.19, 4.217, 4.224, 4.218] median 4.21
    osm pp1 speedup vs pcp per round [0.9891, 1.0002, 1.0106, 0.9983, 0.9909, 1.0] median 0.9992
    osm pp2 speedup vs pcp per round [0.9924, 0.9902, 1.0007, 0.9858, 0.9839, 0.9893] median 0.9898
pp1 ≈ 0 / ≈ 0; pp2 −0.2% / −1.0% → rejected (the lookup stage's own windowed prefetches already cover these lines).
