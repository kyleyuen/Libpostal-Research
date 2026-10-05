# Experiment 037 — lockstep (interleaved) first-token trie walks in phrase search

Date/time/timezone: created 2026-10-02T16:21:15-0700; builds 16:29–16:30, correctness 16:30, timing 2026-10-02T16:31:16-0700 to 2026-10-02T16:42:35-0700, America/Los_Angeles.
Status: **rejected (slower)**

## Hypothesis and change

036 counted ~34 / ~19 node transitions per address in the 8.2M / 7.5M-node component and postal-code tries at ~21–27 ns each, 77–88% of them in first-token walks from the root, and read that as dependent memory latency. Change (switch `EXP_LOCKSTEP`, `sources/trie_search.c`, diff vs `final/021-l3/sources` = 73 added lines): before the token search, walk every token from the start node in lockstep (one byte position across all tokens at a time) and store the (node id, node) pairs; the unchanged search state machine reads them instead of loading nodes whenever it starts a token from a fresh state. Same nodes, same decisions; no new tables. `EXP_LOCKSTEP_VERIFY` runs every search with and without the precomputed walks and aborts on any difference.

## Checks

`lkv` (verify, -O2): 38,589 + 38,589 + 34,215 searches on golden-v1, novel and OSM and 5,365,332 searches over 2,000,000 fuzzed inputs (seed 1): **0 mismatches**; fuzz output identical to the original's (`raw/lkv_*`). `lk` (PGO/LTO/mimalloc as `l3`): output byte-identical to the original on golden-v1 (sha 45574ee2…), novel (806b8a81…) and OSM (`raw/lk_*`). One thread at six samples (`raw/threads.txt`).

## Results (sequential timing, 5 rounds, 20 passes; `raw/analyze.stdout`, 45 runs exit 0)

| binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 33.686 | 1.000× (1.00–1.00) | 34.286 | 1.000× (1.00–1.00) | 21.296 | 1.000× (1.00–1.00) |
| l3 (deliverable) | 7.865 | 4.286× (4.15–4.33) | 7.939 | 4.328× (4.28–4.39) | 4.987 | 4.253× (4.21–4.33) |
| lk = l3 + lockstep walks | 8.081 | 4.168× (4.13–4.19) | 8.159 | 4.200× (4.16–4.24) | 5.264 | 4.049× (4.02–4.08) |

peak footprint bytes (max over rounds): orig 2613217536/2613610728/2612775144; l3 2380335336/2380351744/2370291920; lk 2380335336/2380335336/2370291920

Steady footprint (`raw/steady.txt`): original 2,492 / 2,492 MB, `l3` 2,141 / 2,141 MB, `lk` 2,141 / 2,141 MB. Note: the original measured 2,172 MB earlier the same day (031–032) and 2,492 MB here; `l3` is below both states.

## Decision

Rejected: `lk` is 2.8% / 3.0% / 4.8% slower than `l3` (golden-v1 / novel / OSM). The per-token walks were evidently already overlapped by the out-of-order core; precomputing every token's walk (including tokens the search never starts fresh) adds work without removing stall time. Together with 004, 012 iC and 027, this is the fourth trie-walk latency idea that did not help, so phrase-search time is not dominated by exposed dependent misses (the 036 per-step ns figure was a time/count ratio, not a miss measurement).
