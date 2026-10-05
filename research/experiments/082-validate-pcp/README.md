# Experiment 082 — validation of pcp (new deliverable)

Date/time/timezone: created 2026-10-03T03:23:17-0700; timing 2026-10-03T03:23:23-0700 → 2026-10-03T03:35:09-0700, America/Los_Angeles. Status: **complete — pcp validated; packaged as `research/final/077-pcp`**

Command: `caffeinate -i /bin/bash tools/run_timing.sh` (064's protocol: sequential, 5 rotating rounds, 20 passes, AC-gated; original / lf / pcp on golden-v1, novel, OSM; 45 runs exit 0), then `tools/analyze.py`. Output identity of pcp: 077 `raw/verify_pcp/summary.txt` (all sets, frozen references, 2×2M fuzz) and 080 (unseen golden-v5).

| binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 33.354 | 1.000× (1.00–1.00) | 33.802 | 1.000× (1.00–1.00) | 21.042 | 1.000× (1.00–1.00) |
| lf (previous deliverable) | 6.793 | 4.914× (4.89–4.94) | 6.970 | 4.836× (4.79–4.89) | 4.291 | 4.902× (4.85–4.96) |
| pcp = lf + 067 + 072 + 074 pf2 + 077 pcp | 6.479 | 5.149× (5.13–5.19) | 6.665 | 5.072× (5.01–5.13) | 4.168 | 5.058× (5.03–5.09) |

peak footprint bytes (max over rounds): orig 2613233896/2613135592/2612644048; lf 2537376120/2537392528/2543438200; pcp 2537392528/2537376120/2543438200

Threads (`raw/threads.txt`): 1 at all six samples. Steady footprint (`raw/steady/steady.txt`): original 2,492 / 2,492 MB, lf 1,878 / 1,878 MB, pcp 1,878 / 1,878 MB.
