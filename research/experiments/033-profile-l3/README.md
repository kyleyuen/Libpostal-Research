# Experiment 033 — profile of the `l3` deliverable: where context fill spends its time (diagnostic only)

Date/time/timezone: record created 2026-10-02 (see `raw/created.txt`); run 15:42:43–15:44:57 PDT (`raw/start.txt`, `raw/DONE`), America/Los_Angeles.
Status: complete (measurement only; no speed claim)

## Purpose and change

Pick the next speed target from fresh evidence on the deliverable. (1) `sample` profile of the unchanged shipped `l3` (`021-final-stack/artifacts/l3/bench_ext`, 500 passes of the development split, sampled 15 s at 1 ms after 20 s; `tools/run_sample.sh`, a copy of 026's with the binary and output path changed). (2) Diagnostic stats build `l3cf` = `l3` sources + `EXP_STAGE_STATS` + new switch `EXP_CF_STATS`, which adds five timers inside `address_parser_context_fill` (diff vs `final/021-l3/sources` shown in the session: only `#ifdef EXP_CF_STATS` blocks; `raw/base_sources.sha256` = sources before the edit). Same PGO/LTO/mimalloc procedure (`tools/build_cc.sh`; PGO on golden-v1 development as for `l3`). `tools/stage_bench.c` = 016's driver plus the five sub-stage lines. Command: `caffeinate -i /bin/bash tools/run.sh`; all steps exit 0 (`raw/run.log`). Golden-v1 development split only.

## Results

Stage µs/address (`raw/stage_r{1,2,3}.stdout`; timers included, diagnostic): context fill 2.311 / 2.233 / 2.196, features 2.10 / 2.05 / 2.03, lookups 1.19 / 1.19 / 1.16, accumulation 1.12 / 1.11 / 1.09, Viterbi+rest 1.03 / 1.02 / 1.02 (same picture as 029 for `l4`).

Inside context fill (r1 / r2 / r3):

| sub-stage | µs/address | share of context fill |
|---|---|---:|
| token normalization (standard + admin) | 0.278 / 0.273 / 0.275 | 12% |
| address-dictionary phrase search | 0.288 / 0.272 / 0.267 | 12% |
| prefix/suffix search (per token) | 0.219 / 0.215 / 0.216 | 10% |
| **component-phrase trie search + validity** | **1.034 / 1.010 / 0.981** | **45%** |
| postal-code trie search | 0.466 / 0.451 / 0.444 | 20% |

The validity check (`is_valid_component_phrase`) is a digit scan, so the component cost is the trie walk. Model files: `address_parser_phrases.dat` 137 MB, `address_parser_postal_codes.dat` 622 MB (includes postcode context data). Sample profile (`raw/profile/parser.sample.txt`, summary `raw/profile_report.stdout`, 12,836 samples, host load average 8.6–10.0 in `raw/profile/environment.txt`): `trie_search_tokens_from_index` 16.4% exclusive / 18.2% inclusive, `crf_dict_lookup_array` 11.1%, `address_parser_parse` (LTO-inlined) 27.9%, `memmove` 5.7%, `memset` 3.6%, `strlen` 3.5%, `strncmp` 1.8% — consistent with 026's `l4` profile.

Interpretation (UNVERIFIED cause): about 1.45 µs/address (≈18% of parse time) goes to walking the two large phrase tries, one dependent node read per byte. 027 showed the cost is not the first-byte step. A likely cause is dependent cache/TLB misses deeper in the walks; node-visit counts and miss counters were not collected.

## Decision

Next candidate target: component and postal-code phrase search (≈1.45 µs ceiling). Options (model-derived only): reduce dependent node reads (e.g. a hashed whole-token/phrase index built from the trie keys, with the trie kept as exact fallback) — requires reproducing `trie_search_tokens_from_index` semantics exactly (longest match, partial-match restart, tails, whitespace tokens) and fitting the ~31 MB steady-memory margin of `l3`. A node-visit/miss count is the cheap first step. Decision with the user.
