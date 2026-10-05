# Experiment 121 — validation of cpi (rt8 + 119 component-phrase feature ids)

Date/time/timezone: created per `raw/created.txt`; timing 2026-10-04T01:38:34-0700 → 01:50:23-0700 (`raw/timing_start.txt`, `raw/DONE`), America/Los_Angeles. Status: complete — **cpi is the new deliverable**

Protocol as 103/107/116, cleaned machine; the golden-v11 Overpass fetch ran concurrently (network only). 45 timing commands exited 0.

| binary | v1 µs | v1 speedup (range) | novel µs | novel speedup (range) | OSM µs | OSM speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 31.371 | 1.000× | 32.039 | 1.000× | 19.323 | 1.000× |
| rt8 (previous deliverable) | 5.334 | 5.898× (5.85–5.98) | 5.546 | 5.806× (5.69–5.86) | 3.522 | 5.512× (5.45–5.51) |
| cpi | 5.173 | **6.077×** (6.05–6.18) | 5.404 | **5.928×** (5.86–6.03) | 3.334 | **5.775×** (5.75–5.83) |

- Unseen golden-v10 (120): cpi 5.68×, +4.7% over rt8 in every round.
- Threads: 1.
- Peak footprint: original 2,613,561,552 B; cpi 2,543,421,816 B (unchanged from rt8; the 119 table fits under the load-time peak).
- Steady footprint: original 2,492 MB; rt8 1,878 MB; **cpi 1,927 MB** (+49 MB for the table, still 565 MB below the original).
- Outputs: identical on golden-v1, novel, OSM, unseen v4 and v10, frozen references and fuzz seeds 1 and 2 (119, 120).
- Load: +2.2 s for the table build. Load is not part of the timed metric.
- Packaged as `research/final/119-cpi`.
