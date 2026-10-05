# Experiment 133 — 099's phrase join with known lengths (EXP_PHRASE_LEN), re-tested on cpi

Date/time/timezone: created 2026-10-04T03:06:07-0700 (`raw/created.txt`); screen 2026-10-04T03:07:17-0700 → 2026-10-04T03:13:12-0700; America/Los_Angeles. Status: **inconclusive (below threshold); not carried forward**

Why: in 127/132 `cstring_array_get_phrase` is the largest memmove/strlen caller on OSM (about 1.3% of warm time), and string work is a larger share after 119. Change: 099's helper and its 7 call sites ported to cpi sources (`raw/address_parser.diff`). plc is identical on all sets and fuzz seeds 1 and 2.
Screen vs cpi (`raw/screen_analyze2.stdout`):
        v1 median cpi 5.215 plc 5.196; plc speedup per round [0.9994, 1.0019, 1.0089, 1.0089, 0.9894, 1.0087] median 1.0053; rounds faster 4/6
        v1 min    cpi 5.123 plc 5.091; plc speedup per round [1.0051, 0.9994, 1.0127, 1.0151, 0.9895, 1.014] median 1.0089; rounds faster 4/6
        osm median cpi 3.38 plc 3.344; plc speedup per round [1.0079, 1.0074, 1.0053, 1.0105, 1.023, 1.0137] median 1.0092; rounds faster 6/6
        osm min    cpi 3.274 plc 3.268; plc speedup per round [1.0025, 1.0052, 0.9888, 0.9855, 1.0086, 1.0141] median 1.0038; rounds faster 4/6
+0.5% golden-v1, +0.9% OSM (6/6 by median): consistent with 099, real but below the 1.5% threshold. Not carried forward.
