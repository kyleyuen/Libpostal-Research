# Experiment 094 — validation of vmx (new deliverable)

Date/time/timezone: created 2026-10-03T11:19:40-0700; timing 2026-10-03T11:19:40-0700 → 2026-10-03T11:31:23-0700, America/Los_Angeles. Status: **complete — vmx validated; packaged as `research/final/091-vmx`**

082's protocol (`tools/run_timing.sh`, 45 runs exit 0; `tools/analyze.py`): original / pcp / vmx on golden-v1, novel, OSM. Output identity: 091 checker builds (all sets, frozen references, 2×2M fuzz) and 093 (unseen golden-v7, PGO build).

| binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 33.400 | 1.000× (1.00–1.00) | 34.003 | 1.000× (1.00–1.00) | 21.014 | 1.000× (1.00–1.00) |
| pcp (previous deliverable) | 6.482 | 5.170× (5.13–5.20) | 6.669 | 5.095× (5.07–5.11) | 4.186 | 5.021× (5.00–5.05) |
| vmx = pcp + 091 max-only Viterbi | 6.346 | 5.264× (5.20–5.31) | 6.555 | 5.190× (5.17–5.20) | 4.048 | 5.181× (5.14–5.22) |

peak footprint bytes (max over rounds): orig 2613381352/2613102848/2612938984; pcp 2537392528/2537376120/2543438200; vmx 2537376144/2537359736/2543438224

Threads: 1 at all six samples (`raw/threads.txt`). Steady footprint (`raw/steady/steady.txt`): original 2,492 MB, pcp 1,878 MB, vmx 1,878 MB (twice each).
