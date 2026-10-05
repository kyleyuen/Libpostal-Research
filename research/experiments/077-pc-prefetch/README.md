# Experiment 077 — prefetch postcode-context rows after the postal-code search (EXP_PC_PREFETCH)

Date/time/timezone: created 2026-10-03T02:39:46-0700, America/Los_Angeles. Status: **accepted (stack head pcp = pf2 + this)**

Change (`sources/address_parser.c`, hint only): right after the postal-code phrase search in context fill, prefetch `pc_off[id]` for every postal-code phrase, then read it and prefetch the row start `pc_bytes[pc_off[id]]`; feature generation later decodes these rows (`pc_has_edge`). Motivation: 076 (postcode section 0.27 µs first-touch).

Checks (`raw/verify_pcp/summary.txt`): identical on golden-v1, novel, OSM, unseen v4, frozen references, fuzz seeds 1 and 2.
Timing: a first screen was **discarded** because 078's builds and checks ran concurrently (`raw/contaminated_screen1/`). Clean sequential screen run together with 078 (6 rounds; `../078-vocab-memo-fill/raw/screen_analyze.stdout`):
    v1 pf2 [6.899, 6.622, 6.605, 6.676, 6.636, 6.656] median 6.646
    v1 pcp [6.741, 6.571, 6.529, 6.547, 6.551, 6.564] median 6.558
    v1 vmf [6.924, 6.67, 6.683, 6.673, 6.666, 6.686] median 6.678
    v1 vmr [6.971, 6.734, 6.701, 6.739, 6.692, 6.702] median 6.718
    v1 pcp speedup vs pf2 per round [1.0234, 1.0078, 1.0116, 1.0197, 1.013, 1.014] median 1.0135
    v1 vmf speedup vs pf2 per round [0.9964, 0.9928, 0.9883, 1.0004, 0.9955, 0.9955] median 0.9955
    v1 vmr speedup vs pf2 per round [0.9897, 0.9834, 0.9857, 0.9907, 0.9916, 0.9931] median 0.9902
    osm pf2 [4.262, 4.255, 4.25, 4.29, 4.289, 4.241] median 4.258
    osm pcp [4.25, 4.22, 4.245, 4.223, 4.242, 4.175] median 4.232
    osm vmf [4.31, 4.29, 4.306, 4.286, 4.287, 4.335] median 4.298
    osm vmr [4.32, 4.321, 4.358, 4.324, 4.327, 4.297] median 4.322
    osm pcp speedup vs pf2 per round [1.0028, 1.0083, 1.0012, 1.0159, 1.0111, 1.0158] median 1.0097
    osm vmf speedup vs pf2 per round [0.9889, 0.9918, 0.987, 1.0009, 1.0005, 0.9783] median 0.9904
    osm vmr speedup vs pf2 per round [0.9866, 0.9847, 0.9752, 0.9921, 0.9912, 0.987] median 0.9868
pcp: +1.35% golden-v1, +1.0% OSM, faster in all 12 rounds.
Mechanism (stage + feature-section timers, pf2 vs pcp, 3 alternating runs; `raw/split_analyze.stdout`): postcode section −0.161 µs cold, context fill +0.041 (the hint pass), total −0.114 µs; warm unchanged.
Unseen re-measure on golden-v5 pending.
