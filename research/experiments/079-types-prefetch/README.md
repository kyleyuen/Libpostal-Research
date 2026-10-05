# Experiment 079 — prefetch component phrase-type entries after the component search (EXP_TYPES_PREFETCH)

Date/time/timezone: created 2026-10-03T02:58:15-0700, America/Los_Angeles. Status: **rejected (workload-dependent: gain did not appear on OSM or unseen data)**

Change (hint only): after the component-phrase search, prefetch `parser->phrase_types->a[data]` for each component phrase (read later by the component feature section, 076). golden-v1 sha identical.
Screen vs pcp (6 rounds; `raw/screen_analyze.stdout`; the golden-v5 network fetch was idle-waiting in the background):
    v1 pcp [6.504, 6.488, 6.478, 6.502, 6.497, 6.485] median 6.492
    v1 ptp [6.403, 6.389, 6.418, 6.392, 6.413, 6.394] median 6.399
    v1 ptp speedup vs pcp per round [1.0158, 1.0155, 1.0093, 1.0172, 1.0131, 1.0142] median 1.0149
    osm pcp [4.166, 4.172, 4.16, 4.167, 4.19, 4.178] median 4.169
    osm ptp [4.173, 4.159, 4.164, 4.166, 4.263, 4.196] median 4.17
    osm ptp speedup vs pcp per round [0.9983, 1.0031, 0.999, 1.0002, 0.9829, 0.9957] median 0.9987
+1.5% golden-v1 but neutral on OSM; on the unseen golden-v5 set (080) ptp is 0.6% slower than pcp → rejected. The hint is model-derived (nothing is fitted); its effect is workload-dependent and the −0.6% on v5 is near the noise floor, so it is not shown to help in general.
