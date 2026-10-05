# Experiment 085 — prefetch every line of each weight row (EXP_ROW_LINES), not only the first

Date/time/timezone: created 2026-10-03T03:50:54-0700, America/Los_Angeles. Status: **rejected — rl8 is 1.6% slower on unseen golden-v6 (086); gain specific to Senzing-derived data**

Base pcp. The lookup stage's `EXP_WEIGHT_PREFETCH` loop prefetched only the first 64-byte line of each feature's weight row; state rows span up to 4 lines and transition rows (L×L = 400 cells) much more. Change (`sources/crf.c`, hints only): prefetch every line of each row up to EXP_ROW_LINES lines (rl4 = 4, rl8 = 8; the two values were chosen before measuring, not swept). Further variants: rs8 (multi-line for state rows only), pip (no separate loop; the same prefetches issued inside the accumulation loop 24 state / 6 transition keys ahead).

Outputs: all variants golden-v1 sha identical; rl8 full verification (`raw/verify_rl8/summary.txt`): golden-v1, novel, OSM, unseen v4, frozen references, fuzz seeds 1 and 2 identical.

Screen 1 (6 rounds; `raw/screen1_analyze.stdout`):
    v1 pcp [6.502, 6.491, 6.497, 6.495, 6.494, 6.482] median 6.495
    v1 rl4 [6.271, 6.273, 6.27, 6.28, 6.269, 6.255] median 6.271
    v1 rl8 [6.202, 6.227, 6.19, 6.19, 6.185, 6.202] median 6.196
    v1 rl4 speedup vs pcp per round [1.0368, 1.0348, 1.0362, 1.0342, 1.0359, 1.0363] median 1.036
    v1 rl8 speedup vs pcp per round [1.0484, 1.0424, 1.0496, 1.0493, 1.05, 1.0451] median 1.0488
    osm pcp [4.207, 4.176, 4.166, 4.2, 4.192, 4.186] median 4.189
    osm rl4 [4.21, 4.209, 4.247, 4.247, 4.203, 4.219] median 4.215
    osm rl8 [4.208, 4.213, 4.194, 4.199, 4.199, 4.216] median 4.204
    osm rl4 speedup vs pcp per round [0.9993, 0.9922, 0.9809, 0.9889, 0.9974, 0.9922] median 0.9922
    osm rl8 speedup vs pcp per round [0.9998, 0.9912, 0.9933, 1.0002, 0.9983, 0.9929] median 0.9958
Mechanism (stage timers pcp vs rl8, 3 alternating runs each on development and OSM; `raw/split_analyze.stdout`):
    dev
      total_parse        pcp cold 6.845 rl8 cold 6.574 Δ -0.271 | warm 3.072 3.113 Δ +0.041
      context_fill       pcp cold 2.132 rl8 cold 2.113 Δ -0.019 | warm 0.524 0.522 Δ -0.002
      features           pcp cold 1.560 rl8 cold 1.552 Δ -0.008 | warm 0.812 0.811 Δ -0.001
      lookups            pcp cold 0.852 rl8 cold 1.058 Δ +0.206 | warm 0.268 0.322 Δ +0.054
      accumulate         pcp cold 1.105 rl8 cold 0.648 Δ -0.457 | warm 0.516 0.495 Δ -0.021
      viterbi_and_rest   pcp cold 0.658 rl8 cold 0.664 Δ +0.006 | warm 0.618 0.619 Δ +0.001
    osm
      total_parse        pcp cold 4.530 rl8 cold 4.575 Δ +0.045 | warm 2.653 2.686 Δ +0.033
      context_fill       pcp cold 1.259 rl8 cold 1.260 Δ +0.001 | warm 0.468 0.465 Δ -0.003
      features           pcp cold 1.023 rl8 cold 1.033 Δ +0.010 | warm 0.660 0.660 Δ +0.000
      lookups            pcp cold 0.516 rl8 cold 0.687 Δ +0.171 | warm 0.220 0.268 Δ +0.048
      accumulate         pcp cold 0.715 rl8 cold 0.568 Δ -0.147 | warm 0.475 0.457 Δ -0.018
      viterbi_and_rest   pcp cold 0.584 rl8 cold 0.586 Δ +0.002 | warm 0.552 0.554 Δ +0.002
Accumulation's first-touch cost falls sharply (−0.46 µs development, −0.15 µs OSM) while the prefetch loop adds ≈0.2 µs to the lookup stage; on OSM-style data (shorter addresses, accumulation mostly cached) the two cancel.
Screen 2 (6 rounds; `raw/screen2_analyze.stdout`):
    v1 pcp [6.484, 6.5, 6.496, 6.489, 6.472, 6.497] median 6.492
    v1 rl8 [6.193, 6.204, 6.188, 6.203, 6.216, 6.193] median 6.198
    v1 rs8 [6.426, 6.424, 6.409, 6.413, 6.424, 6.422] median 6.423
    v1 pip [6.248, 6.24, 6.249, 6.269, 6.251, 6.247] median 6.248
    v1 rl8 speedup vs pcp per round [1.047, 1.0477, 1.0498, 1.0461, 1.0412, 1.0491] median 1.0473
    v1 rs8 speedup vs pcp per round [1.009, 1.0118, 1.0136, 1.0119, 1.0075, 1.0117] median 1.0118
    v1 pip speedup vs pcp per round [1.0378, 1.0417, 1.0395, 1.0351, 1.0354, 1.04] median 1.0386
    osm pcp [4.195, 4.186, 4.191, 4.158, 4.156, 4.229] median 4.188
    osm rl8 [4.206, 4.171, 4.188, 4.215, 4.185, 4.221] median 4.197
    osm rs8 [4.255, 4.235, 4.22, 4.246, 4.237, 4.216] median 4.236
    osm pip [4.247, 4.264, 4.25, 4.231, 4.235, 4.24] median 4.244
    osm rl8 speedup vs pcp per round [0.9974, 1.0036, 1.0007, 0.9865, 0.9931, 1.0019] median 0.9991
    osm rs8 speedup vs pcp per round [0.9859, 0.9884, 0.9931, 0.9793, 0.9809, 1.0031] median 0.9872
    osm pip speedup vs pcp per round [0.9878, 0.9817, 0.9861, 0.9827, 0.9813, 0.9974] median 0.9844
rl8 +4.7% golden-v1, −0.1% OSM (within noise); rs8 shows the gain comes mostly from transition rows; pip and rs8 lose on OSM.

## Decision
Unseen golden-v6 (086): rl8 −1.6% vs pcp (5/5 rounds). The multi-line row prefetch helps when weight rows are mostly uncached (longer Senzing-derived addresses) and costs ≈0.2 µs of prefetch issue when they are not (OSM-style data); a general benefit is not shown → rejected. pip and rs8 were already slower on OSM.
