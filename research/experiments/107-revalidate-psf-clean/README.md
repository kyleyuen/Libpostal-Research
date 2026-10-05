# Experiment 107 — re-validation of psf on the cleaned machine

Date/time/timezone: created per `raw/created.txt`; timing 2026-10-03T23:52:48-0700 → 2026-10-04T00:03:54-0700 (`raw/timing_start.txt`, `raw/DONE`), America/Los_Angeles. Status: complete

Same protocol and binaries as 103 (`tools/run_timing.sh`, `tools/analyze.py`). The difference is the machine state: four long-running background CPU users were removed beforehand (ACTIVITY_LOG 2026-10-03 23:29–23:47; 106 A/A check):
- two orphaned VS Code crash reporters at ≈170% CPU each for 30+ days;
- a leftover ASan test binary from an earlier session;
- `icdd`.
The golden-v10 fetch ran concurrently (network only). 45 timing commands exited 0.

`raw/analyze.stdout`:

| binary | v1 µs | v1 speedup (range) | novel µs | novel speedup (range) | OSM µs | OSM speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 31.293 | 1.000× | 31.776 | 1.000× | 19.196 | 1.000× |
| vmx | 5.649 | 5.549× (5.50–5.60) | 5.855 | 5.442× (5.41–5.46) | 3.722 | 5.157× (5.08–5.21) |
| psf | 5.439 | **5.753×** (5.71–5.77) | 5.650 | **5.649×** (5.58–5.68) | 3.586 | **5.323×** (5.28–5.45) |

- Threads: 1.
- Peak: original 2.613 GB, psf 2.543 GB.
- Steady: 2,492 vs 1,878 MB.
- Effect of the cleanup: compared with 103 (same binaries, loaded machine), the original got 7–10% faster and psf 12–14% faster. The background load had cost the optimized build more, presumably through shared-cache contention (045 already suspected this; UNVERIFIED mechanism).
- Golden-v1 and novel speedups rose (5.44 → 5.75×, 5.39 → 5.65×). OSM is about the same (5.45 → 5.32×; the original sped up more on OSM).
- Both sessions are kept. Each is internally consistent.
