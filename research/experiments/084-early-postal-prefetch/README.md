# Experiment 084 — postal-trie prefetch issued a stage early (EXP_EARLY_POSTAL[2])

Date/time/timezone: created 2026-10-03T03:41:33-0700, America/Los_Angeles. Status: **rejected (slower)**

Base pcp. Hints only (`sources/address_parser.c` `ap_ep_stage1/2`): after token normalization, prefetch every admin token's second-level postal-trie node (ep1); ep2 additionally, after the address-dictionary search, reads those nodes and prefetches the third-level node or leaf data entry. (Advisor suggestion: 012's per-search hint failed at short distance; issue it ≈1.5 µs earlier.) golden-v1 sha identical.
Screen (6 rounds; `raw/screen_analyze.stdout`):
    v1 pcp [6.489, 6.514, 6.5, 6.486, 6.494, 6.485] median 6.492
    v1 ep1 [6.507, 6.505, 6.51, 6.496, 6.498, 6.502] median 6.503
    v1 ep2 [6.581, 6.491, 6.507, 6.516, 6.482, 6.525] median 6.511
    v1 ep1 speedup vs pcp per round [0.9972, 1.0014, 0.9985, 0.9985, 0.9994, 0.9974] median 0.9985
    v1 ep2 speedup vs pcp per round [0.986, 1.0035, 0.9989, 0.9954, 1.0019, 0.9939] median 0.9972
    osm pcp [4.165, 4.194, 4.18, 4.168, 4.177, 4.189] median 4.178
    osm ep1 [4.211, 4.197, 4.227, 4.19, 4.185, 4.204] median 4.2
    osm ep2 [4.226, 4.216, 4.218, 4.19, 4.21, 4.197] median 4.213
    osm ep1 speedup vs pcp per round [0.9891, 0.9993, 0.9889, 0.9947, 0.9981, 0.9964] median 0.9956
    osm ep2 speedup vs pcp per round [0.9856, 0.9948, 0.991, 0.9947, 0.9922, 0.9981] median 0.9935
ep1 −0.15% / −0.4%, ep2 −0.3% / −0.65% → rejected; the postal-code search is not limited by these misses.
