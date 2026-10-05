# Experiment 130 — address-dictionary phrase features as precomputed ids (EXP_DP_IDS; 119's method)

Date/time/timezone: created 2026-10-04T02:51:04-0700 (`raw/created.txt`); screen 2026-10-04T02:54:50-0700 → 2026-10-04T03:00:45-0700; America/Los_Angeles. Status: **inconclusive (below threshold); not carried forward**

Change (`raw/address_parser.diff`): the ten dictionary-phrase APF calls become `ap_dp_emit`. At load, every address-dictionary key (153,670) is run through it with its expansion's components, and the ids found are stored (30,200 ids). At parse time the length guard picks the table. A second key for any data id disables the table for that id (robustness; none occur in this model).
Correctness: `dpv` (+ `EXP_DP_IDS_VERIFY`: enumeration check and every table use re-derived through the string path) and `dpi` identical on all sets and fuzz seeds 1 and 2. About 5.05M uses verified, 0 mismatches.
Screen vs cpi (`raw/screen_analyze2.stdout`):
        v1 median cpi 5.255 dpi 5.207; dpi speedup per round [1.0004, 1.0186, 1.0127, 1.0104, 0.9973, 1.0147] median 1.0115; rounds faster 5/6
        v1 min    cpi 5.145 dpi 5.09; dpi speedup per round [1.0, 1.0307, 1.0052, 1.0016, 1.0029, 1.0113] median 1.004; rounds faster 5/6
        osm median cpi 3.367 dpi 3.396; dpi speedup per round [1.0071, 1.0045, 0.9762, 0.9841, 0.9923, 1.0074] median 0.9984; rounds faster 3/6
        osm min    cpi 3.282 dpi 3.301; dpi speedup per round [0.9811, 1.0098, 0.9973, 0.9861, 0.9875, 1.0018] median 0.9924; rounds faster 2/6
Decision: +1.1% (median) / +0.4% (min) on golden-v1 and −0.2% / −0.8% on OSM, below the 1.5% threshold. The family is about 2 keys per address (123), too small to measure. Not carried forward.
