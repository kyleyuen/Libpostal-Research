# Experiment 057 — full same-session validation of 056 `ftr` (va + postcode-graph compression + first-token table + role-row prefetch)

Date/time/timezone: created 2026-10-02T18:53:28-0700; timing 2026-10-02T18:54:12-0700 to 2026-10-02T19:05:43-0700 (host clock), America/Los_Angeles.
Status: **not adopted (mixed: faster on golden-v1 and novel, slower on OSM)**

Correctness (`raw/verify_summary.txt`): byte-identical to the original on golden-v1, novel and OSM, both frozen references, 2 × 2,000,000 fuzz; one thread (`raw/threads.txt`). Hashes `raw/ftr_hashes.txt`.

Speed (sequential, 5 rotating rounds, 20 passes, paired with the original; `raw/analyze.stdout`):
    | binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
    |---|---:|---:|---:|---:|---:|---:|
    | original | 33.572 | 1.000× (1.00–1.00) | 34.215 | 1.000× (1.00–1.00) | 21.207 | 1.000× (1.00–1.00) |
    | va (deliverable) | 7.431 | 4.513× (4.50–4.54) | 7.517 | 4.554× (4.53–4.56) | 4.613 | 4.585× (4.56–4.62) |
    | ftr = va + postcode compression + first-token table + role prefetch | 7.256 | 4.624× (4.62–4.65) | 7.398 | 4.624× (4.60–4.63) | 4.662 | 4.551× (4.51–4.56) |
    
    peak footprint bytes (max over rounds): orig 2613807360/2613430504/2612725968; va 2380335336/2380335336/2370291920; ftr 2537376120/2537392528/2543438200

Memory: peak `ftr` 2,537–2,543 MB (load-time temporaries of the graph compression and table build) vs original 2,612–2,614 MB; steady `ftr` 1,878 MB vs `va` 2,141 MB vs original 2,492 MB (`raw/steady/steady.txt`).

Decision: `ftr` is +2.5% (golden-v1) and +1.5% (novel) over `va` but −0.7% on OSM (likely the gap-decoding cost of the compressed postcode graph on postcode-heavy OSM addresses; not isolated). Without the compression the table does not fit the steady-memory rule (`va` 2,141 MB + 56 MB > the original's 2,172 MB lower state). `va` remains the deliverable.
