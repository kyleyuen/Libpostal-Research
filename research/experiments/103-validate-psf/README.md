# Experiment 103 — validation of psf (vmx + 098 paired normalization + 101 prefix/suffix flags)

Date/time/timezone: created per `raw/created.txt`; timing 2026-10-03T22:41:13-0700 → 22:53:02-0700 (`raw/timing_start.txt`, `raw/DONE`), America/Los_Angeles. Status: complete — **psf is the new deliverable**

Protocol as 094 (`tools/run_timing.sh`, `tools/analyze.py`): the original, vmx (previous deliverable) and psf were timed sequentially, in rotating order, AC-gated, 5 rounds × 20 passes on golden-v1, novel and OSM. Then a thread check (`ps -M`) and the steady footprint (`footprint -p`, two samples each). The golden-v9 Overpass fetch ran concurrently (network only). Unrelated long-running processes were using CPU as in 102 (orphaned VS Code crashpad handlers, `icdd`).

`raw/analyze.stdout`:

| binary | v1 µs | v1 speedup (range) | novel µs | novel speedup (range) | OSM µs | OSM speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 33.680 | 1.000× | 34.544 | 1.000× | 21.407 | 1.000× |
| vmx (previous deliverable) | 6.408 | 5.267× (5.19–5.29) | 6.582 | 5.236× (5.16–5.25) | 4.114 | 5.194× (5.14–5.22) |
| psf = vmx + 098 + 101 | 6.200 | **5.437×** (5.38–5.50) | 6.403 | **5.386×** (5.37–5.64) | 3.937 | **5.453×** (5.41–5.48) |

- Unseen golden-v8 (102): **5.37×** (vmx 5.13× in the same session).
- Threads: 1 at all 6 samples (`raw/threads.txt`).
- Peak footprint (max over rounds): original 2,613,643,496 B, vmx 2,543,421,816 B, psf 2,543,421,816 B.
- Steady footprint: original 2,492 MB, vmx 1,878 MB, psf 1,878 MB (`raw/steady/steady.txt`).
- Output identity: see 098, 101 and 102 (golden-v1, novel, OSM, unseen v4 and v8, frozen references, 2 × 2M fuzz).
Packaged as `research/final/101-psf`.
