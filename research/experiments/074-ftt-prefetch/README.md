# Experiment 074 — prefetch pre-pass for the first-token table walk (EXP_FTT_PF1/PF2)

Date/time/timezone: created 2026-10-03T02:02:27-0700, America/Los_Angeles. Status: **pf2 accepted (stack head); pf3, pfv, psp rejected (no added gain)**

## Motivation (073 profile of the head)
073 (`073-profile-head`) re-ran the cold/warm stage split on the current head: context fill is the largest first-touch bucket (2.22 µs cold vs 0.53 warm); within it the component-phrase search costs 1.04 µs cold (slot loop 0.34, trie search 0.66) even with 056's first-token table. Counters (073 `raw/fst_*.stderr`): per address 8.3 non-whitespace admin tokens, 3.4 assisted by the table; each assisted token reads a table entry (56 MB table) and then the node it names, two dependent random loads, and the tokens' chains are independent of each other.

## Change (hints only; no result depends on them)
In the slot loop of `address_parser_context_fill` (`sources/address_parser.c`):
- `EXP_FTT_PF1`: prefetch `trie_ftt_tab[slot]` as soon as each token's vocabulary slot is known.
- `EXP_FTT_PF2`: after the loop, read every token's entry (independent loads) and prefetch the node `fin` the search will load first.
Also tried: `EXP_FTT_PF3` (+ read `fin` and prefetch its '\\0'/' ' transition cells), `EXP_FTT_PFV` (prefetch every admin token's vocabulary slot before the slot loop), `psp` = pf2 + 012's `EXP_SEARCH_PREFETCH`.

## Checks
pf2 full verification (`raw/verify_pf2/summary.txt`): identical on golden-v1 (45574ee2…), novel (806b8a81…), OSM, unseen golden-v4, frozen references; fuzz seeds 1 and 2 identical. pf1/pf3/pfv/psp: golden-v1 sha identical.

## Results (sequential 6-round screens, 20 passes)
Screen 1 vs dg (`raw/screen1_analyze.stdout`):
    v1 dg [6.779, 6.774, 6.839, 6.798, 6.788, 6.783] median 6.786
    v1 pf1 [6.696, 6.75, 6.689, 6.706, 6.718, 6.683] median 6.701
    v1 pf2 [6.61, 6.65, 6.621, 6.644, 6.636, 6.611] median 6.629
    v1 pf1 speedup vs dg per round [1.0124, 1.0036, 1.0224, 1.0137, 1.0104, 1.015] median 1.0131
    v1 pf2 speedup vs dg per round [1.0256, 1.0186, 1.0329, 1.0232, 1.0229, 1.026] median 1.0244
    osm dg [4.335, 4.282, 4.305, 4.297, 4.31, 4.292] median 4.301
    osm pf1 [4.255, 4.243, 4.263, 4.284, 4.226, 4.271] median 4.259
    osm pf2 [4.251, 4.312, 4.265, 4.271, 4.263, 4.25] median 4.264
    osm pf1 speedup vs dg per round [1.0188, 1.0092, 1.0099, 1.003, 1.0199, 1.0049] median 1.0095
    osm pf2 speedup vs dg per round [1.0198, 0.993, 1.0094, 1.0061, 1.011, 1.0099] median 1.0096
Screen 2 vs pf2 (`raw/screen2_analyze.stdout`):
    v1 pf2 [6.657, 6.663, 6.619, 6.653, 6.638, 6.639] median 6.646
    v1 pf3 [6.621, 6.635, 6.62, 6.672, 6.65, 6.608] median 6.628
    v1 pfv [6.64, 6.643, 6.658, 6.654, 6.635, 6.642] median 6.643
    v1 pf3 speedup vs pf2 per round [1.0054, 1.0042, 0.9998, 0.9972, 0.9982, 1.0047] median 1.002
    v1 pfv speedup vs pf2 per round [1.0026, 1.003, 0.9941, 0.9998, 1.0005, 0.9995] median 1.0002
    osm pf2 [4.275, 4.27, 4.281, 4.282, 4.255, 4.261] median 4.272
    osm pf3 [4.244, 4.292, 4.261, 4.237, 4.253, 4.267] median 4.257
    osm pfv [4.285, 4.281, 4.282, 4.257, 4.29, 4.306] median 4.284
    osm pf3 speedup vs pf2 per round [1.0073, 0.9949, 1.0047, 1.0106, 1.0005, 0.9986] median 1.0026
    osm pfv speedup vs pf2 per round [0.9977, 0.9974, 0.9998, 1.0059, 0.9918, 0.9895] median 0.9975
Screen 3 vs pf2 (`raw/screen3_analyze.stdout`):
    v1 pf2 [6.636, 6.647, 6.634, 6.622, 6.62, 6.645] median 6.635
    v1 psp [6.638, 6.629, 6.683, 6.666, 6.646, 6.673] median 6.656
    v1 psp speedup vs pf2 per round [0.9997, 1.0027, 0.9927, 0.9934, 0.9961, 0.9958] median 0.9959
    osm pf2 [4.267, 4.278, 4.257, 4.263, 4.266, 4.267] median 4.267
    osm psp [4.236, 4.271, 4.265, 4.294, 4.274, 4.274] median 4.272
    osm psp speedup vs pf2 per round [1.0073, 1.0016, 0.9981, 0.9928, 0.9981, 0.9984] median 0.9982

Mechanism (stage timers, identical builds dg vs pf2 + EXP_STAGE_STATS, 3 alternating runs, development split; `raw/split_analyze.stdout`):
    stage                 dg cold pf2 cold   Δcold  dg warm pf2 warm
    total_parse             7.099    6.934  -0.165    3.075    3.069
    normalize               0.073    0.072  -0.001    0.046    0.045
    tokenize                0.242    0.238  -0.004    0.139    0.136
    context_fill            2.219    2.077  -0.142    0.525    0.520
    predict_total           4.330    4.318  -0.012    2.210    2.215
    response                0.212    0.210  -0.002    0.136    0.134
    features                1.714    1.709  -0.005    0.817    0.816
    lookups                 0.858    0.855  -0.003    0.270    0.277
    accumulate              1.097    1.098  +0.001    0.505    0.506
    viterbi_and_rest        0.658    0.657  -0.001    0.618    0.617
Only context fill moves (−0.142 µs cold, warm unchanged), consistent with overlapped first-touch misses; total −0.165 µs (−2.3%) matches the end-to-end screen.

## Decision
pf2 (= dg + PF1 + PF2) accepted as the new stack head: +2.4% golden-v1 (6/6 rounds), +1.0% OSM (5/6), mechanism confirmed. pf3, pfv and psp add nothing → rejected. Unseen-data re-measure on golden-v5 pending (user rule).
