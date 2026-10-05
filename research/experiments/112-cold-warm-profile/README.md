# Experiment 112 — where the first-touch time goes: cold-minus-warm profile of psf (diagnostic)

Date/time/timezone: created 2026-10-04T00:24:41-0700 (`raw/created.txt`), America/Los_Angeles; last output 00:29:45 (`raw/pc_profile_inline.stdout` mtime); commands in `raw/commands.txt`. Status: complete

Method:
1. **Function level** (`tools/run.sh`, `tools/diff_profile.py`). psf `bench_ext` ran two ways: cold, cycling all 12,863 golden-v1 addresses, and warm, cycling 300 development addresses (095's method). `sample <pid> 20 1` ran after 20 s, two runs each, alternating. For each function, µs per address = top-of-stack share × that run's median µs/address; cold − warm, median of the two runs (`raw/diff_profile.stdout`).
2. **Line level** (`tools/pc_profile.py`). One 8 s run each, cold and warm, with `xctrace record --template 'CPU Profiler'`. The cpu-profile table was exported (`raw/xc_*_cpu.xml`; trace bundles kept). Leaf program counters were mapped with `atos -i` to the innermost inlined source line (`raw/pc_profile.stdout`, `raw/pc_profile_inline.stdout`).

Results, function level (cold 5.82 / 5.99, warm 3.59 / 3.65 µs/address): cold − warm sums to 2.28 µs. `address_parser_parse` (inlined code) 0.567; `trie_search_tokens_from_index` 0.439; `crf_dict_lookup_array` 0.296; `fd_find_slot` 0.189; `address_parser_features` 0.167; `memset` 0.133; `strncmp` 0.092; `strlen` 0.085; `memmove` 0.078.
Results, line level (innermost line, cold − warm, µs/address on the xctrace-inflated scale):
- crf.c:674 (state weight-row accumulation) 0.289
- trie_search.c (line 0 = compiler-generated/inlined node reads) 0.278
- crf.c:688 (transition weight-row accumulation) 0.199
- trie_search.c:546 0.121
- feature_dict.h:81 (`fd_next` probe) 0.119
- `pc_has_edge` 0.067
- `fd_find_slot` lines ≈0.13 in total
- crf.c:670 0.060

Caveats:
- xctrace inflates absolute times (cold 6.71 vs 5.8 µs without it).
- The warm run cycles different addresses (dev300) from the cold run, so the difference also contains workload differences.
- Line 0 entries are code without line information.
- Single runs at line level.
Use: this led to 113 (weight-row prefetch re-test).
