# Experiment 116 — validation of rt8 (psf + 114 transition-row every-line prefetch)

Date/time/timezone: created per `raw/created.txt`; timing 2026-10-04T01:03:28-0700 → 01:14:39-0700 (`raw/timing_start.txt`, `raw/DONE`), America/Los_Angeles. Status: complete — **rt8 is the new deliverable**

Protocol as 103/107 (`tools/run_timing.sh`, `tools/analyze.py`), cleaned machine; 45 timing commands exited 0.

| binary | v1 µs | v1 speedup (range) | novel µs | novel speedup (range) | OSM µs | OSM speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 31.463 | 1.000× | 31.940 | 1.000× | 19.285 | 1.000× |
| psf (previous deliverable) | 5.499 | 5.721× (5.71–5.75) | 5.696 | 5.604× (5.56–5.65) | 3.558 | 5.420× (5.34–5.50) |
| rt8 | 5.362 | **5.865×** (5.84–5.89) | 5.531 | **5.769×** (5.71–5.81) | 3.521 | **5.491×** (5.39–5.54) |

- Unseen golden-v9 (115): rt8 5.28×, not slower than psf (+0.7%).
- Threads: 1.
- Peak footprint: original 2,613,971,152 B, rt8 2,543,454,608 B (psf 2,543,438,224 B).
- Steady: original 2,492–2,493 MB, rt8 1,878 MB.
- Outputs: identical on golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2 (`114-trans-rows-prefetch/raw/verify_rt8/summary.txt`), and on unseen v9 (115).
- Packaged as `research/final/114-rt8`.
