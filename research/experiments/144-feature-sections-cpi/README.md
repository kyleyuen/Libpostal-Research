# Experiment 144 — feature-section split of cpi (diagnostic)

Date/time/timezone: created 2026-10-04T20:21:29-0700 (`raw/created.txt`), America/Los_Angeles; timing 2026-10-04T20:26:55-0700 → 20:32:24-0700 (`raw/timing_start.txt`, `raw/DONE`). Status: complete

## Why

User request (2026-10-04): plan the rewrite through math before building. The integer pipeline's main card replaces the bigram string path, and bigrams are built inside the prev-word and next-word sections of `address_parser_features`.

This experiment measures those sections on the current deliverable. The last such measurement was 076, on the older 074 head.

## Change and reproduction

- `sources/address_parser.c` = `final/119-cpi` + 076/038's eight `FS_MARK` timers under `EXP_FS_STATS`. The diff (`raw/address_parser.diff`) has 33 lines added and none changed. Anchors were asserted by script.
- Build `fss` = cpi switches + `EXP_STAGE_STATS` + `EXP_FS_STATS` (`raw/switches_fss.txt`), via 119's `build_cc.sh` (PGO/LTO/mimalloc), with `bench_split_vt.c` linked using `-DHAVE_FS`.
  - Attempt 1 failed because `artifacts/scanner_cache` was missing (`raw/attempt1_*`).
  - Attempt 2 exit 0.
- fss output: golden-v1 sha `45574ee2…`, OSM identical (`raw/identity.txt`).
- Timing: `tools/run.sh` runs fss alternating with 119 `css` on development, OSM and v11, 4 rounds, 3 passes, gated. All 24 runs exit 0. Hygiene is in `raw/hygiene.txt`: the v13 fetch and IDE `clangd` were present but idle.
- Analysis: `tools/analyze.py` → `raw/analyze.stdout`.

## Results (µs per address, cold / warm, median of 4)

| Section (fss) | dev | OSM | u11 |
|---|---|---|---|
| prev word (+ `prev word+word` bigram) | 0.173 / 0.116 | 0.137 / 0.103 | 0.129 / 0.102 |
| next word (+ `word+next word` bigram, right-context) | 0.371 / 0.188 | 0.257 / 0.164 | 0.237 / 0.161 |
| **both neighbour sections** | **0.544 / 0.304** | **0.394 / 0.267** | **0.366 / 0.263** |
| share of fss total cold | 9.8% | 10.7% | 10.6% |
| word / trans word | 0.164 / 0.058 | 0.111 / 0.050 | 0.131 / 0.048 |
| component / postcode / address dict / setup | 0.300 / 0.193 / 0.071 / 0.054 | 0.114 / 0.108 / 0.056 / 0.049 | 0.100 / 0.116 / 0.058 / 0.048 |

The timers add ≈0.06–0.07 µs to features cold and ≈0.09 µs warm (fss vs css). Other stages are unchanged within ±0.03 µs.

The neighbour sections contain more than bigram building:
- neighbour resolution (`AP_WORD_OR_PHRASE`, the vocabulary frequency of the neighbour);
- the unigram neighbour keys (already integer keys in cpi);
- the bigram string build;
- for next word, the right-context features.

They are therefore an upper bound on the bigram string cost. 145 isolates the bigram part.

## Decision

Diagnostic. The input for the model is in experiment 145.
