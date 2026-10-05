# Experiment 064 — full same-session validation of 062 `lf`

Date/time/timezone: created 2026-10-02T20:22:02-0700; timing 2026-10-02T20:22:41-0700 to 2026-10-02T20:34:15-0700 (host clock), America/Los_Angeles.
Status: **accepted — `lf` becomes the deliverable** (`final/062-lf/`)

`lf` = `va` (048) + exact gap-encoded postcode-context graph (053) + first-token table for the component trie and early role-row prefetch (056 `ftr`) + 1-byte name ids in feature keys (059, 061) + all-letters normalization fast path (062). Switches: `062-letters-fastpath/raw/switches.txt`; hashes `raw/lf_hashes.txt`.

Correctness (`raw/verify_summary.txt`): byte-identical to the original on golden-v1, novel and OSM, both frozen references, 2 × 2,000,000 fuzz; one thread at 3–18 s (`raw/threads.txt`).

Speed (sequential, 5 rotating rounds, 20 passes, paired with the original; `raw/analyze.stdout`):
    | binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
    |---|---:|---:|---:|---:|---:|---:|
    | original | 33.278 | 1.000× (1.00–1.00) | 33.669 | 1.000× (1.00–1.00) | 20.826 | 1.000× (1.00–1.00) |
    | va (deliverable) | 7.376 | 4.514× (4.49–4.52) | 7.481 | 4.518× (4.49–4.52) | 4.558 | 4.600× (4.55–4.66) |
    | lf = va + 053 + 056 ftr + 059/061 name bytes + 062 letters | 6.772 | 4.924× (4.88–4.94) | 6.938 | 4.860× (4.83–4.88) | 4.256 | 4.909× (4.86–4.96) |
    
    peak footprint bytes (max over rounds): orig 2613348584/2613414096/2612775144; va 2380335336/2380335336/2370308328; lf 2537376120/2537376120/2543438200

Memory: peak `lf` 2,537–2,543 MB vs original 2,613–2,614 MB; steady `lf` 1,878 MB vs `va` 2,141 MB vs original 2,492 MB (and below the original's 2,172 MB state seen on 2026-10-02) (`raw/steady/steady.txt`). Load time is longer (table and compression builds at load; ≈10–11 s, not timed by the benchmark statistic).

Decision: accepted. 4.92× / 4.86× / 4.91× vs `va`'s 4.51× / 4.52× / 4.60× in the same session.
