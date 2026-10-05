# Experiment 058 — numeric-token table for the postal-code trie; checkpoint index for the compressed postcode graph (both on 056 `ftr`)

Date/time/timezone: created 2026-10-02T19:07:01-0700, America/Los_Angeles. Status: **both rejected**

## A. `EXP_NUMT` — every digit string of 1–6 digits walked through the postal-code trie at load (1.11M entries, direct-indexed by value, model data only); fresh digit tokens use the entry via the 056 mechanism.
Verify build: 63,743 + 60,582 + 52,496 + 10,494,403 (2M fuzz) table-assisted walks checked, 0 mismatches; outputs identical (`raw/verify_summary.txt`). Screen vs `ftr` (`raw/screen_analyze.stdout`):
    v1 ftr [7.278, 7.255, 7.276, 7.276] median 7.276
    v1 nt [7.324, 7.282, 7.293, 7.317] median 7.305
    v1 nt vs ftr per round [0.9937, 0.9963, 0.9977, 0.9944]
    osm ftr [4.695, 4.68, 4.713, 4.688] median 4.691
    osm nt [4.665, 4.773, 4.693, 4.706] median 4.7
    osm nt vs ftr per round [1.0064, 0.9805, 1.0043, 0.9962]
No gain (digit tokens' walks are short and cached). Rejected.

## B. Is 053's compression the OSM cost? `va` vs `pc` on OSM (`raw/screen_pc_osm_analyze.stdout`):
    osm va [4.674, 4.622, 4.647, 4.656] median 4.652
    osm pc [4.692, 4.7, 4.68, 4.76] median 4.696
    osm pc vs va per round [0.9962, 0.9834, 0.9929, 0.9782]
Yes, ≈0.4–2.2%. Row lengths (`raw/rowstats.stderr`): 2.1M rows have >32 entries (89M edges), max 977.

## C. `EXP_PC_SKIP` — checkpoint every 16 entries (value, byte offset; 105.1 MB); lookup = binary search over the row's checkpoints + ≤15 decoded values. Load-time proof against `graph_has_edge` on the original arrays for every edge and both neighbours: **366,843,205 queries, 0 mismatches**; outputs identical (`raw/verify_sk.txt`). Screen `va` / `ftr` / `sk` on golden-v1 and OSM (`raw/screen_sk_analyze.stdout`):
    v1 va [7.46, 7.49, 7.456, 7.459] median 7.46
    v1 ftr [7.236, 7.279, 7.266, 7.27] median 7.268
    v1 sk [7.342, 7.353, 7.382, 7.369] median 7.361
    v1 ftr vs va per round [1.031, 1.029, 1.0261, 1.026]
    v1 sk vs va per round [1.0161, 1.0186, 1.01, 1.0122]
    osm va [4.648, 4.651, 4.642, 4.598] median 4.645
    osm ftr [4.71, 4.606, 4.648, 4.677] median 4.662
    osm sk [4.734, 4.713, 4.715, 4.672] median 4.714
    osm ftr vs va per round [0.9868, 1.0098, 0.9987, 0.9831]
    osm sk vs va per round [0.9818, 0.9868, 0.9845, 0.9842]
`sk` is slower than `ftr` on both (the extra checkpoint arrays add cold reads); decoding was not the bottleneck. Rejected. In this screen `ftr` vs `va`: +2.6–3.1% golden-v1, −1.7…+1.0% OSM.

Housekeeping: the row-statistics scratch build was moved to `raw/rowstats_scratch/` (a deletion was blocked by the workspace guard).
