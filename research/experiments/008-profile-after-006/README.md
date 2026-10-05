# Experiment 008 — Fresh profile after 006 and memory composition (analysis only)

Date: 2026-09-29 PDT. Status: **complete (measurement only; no library change, no speedup claimed)**
Purpose: user asked for a fresh stage profile of the current best build, focusing on runtime and memory-allocation headroom, and to test their (self-labelled unverified) assumption that
runtime has a lot of headroom while allocation has not much.

## Method
`sample` (1 ms, 15 s after READY) on a 25 s run over golden-v1/all for the preserved baseline and the 006 build `d` (`tools/run_sample_bin.sh`), analysed with `tools/profile_report.py` and `tools/callers.py`;
model composition from struct sizes in the loaded parser (`tools/composition.c`, baseline library). **Environment:** the profiles were first taken while the laptop was on battery (the AC gate FAILED then; kept in `raw/battery-run/`, `raw/battery-run/ENV_NOTE.txt`) and were **rerun on AC power** at the user's request (gate OK, battery 91% charging, load average ~7, CPU ~40% busy from unrelated processes; `raw/env-ac-rerun.txt`). The tables below are from the AC rerun.
The two runs agree: every listed share differs by 1 point or less (e.g. `crf_tagger_score` exclusive 13.3% AC vs 13.2% battery, memmove 11.3% vs 11.1%, allocator entry points 6.0% vs 6.8%). They are profiling runs, not timings; each is one 15 s sample.

## Where the time goes now (`raw/profile-d.txt`, AC rerun; % of samples, ~20.7 us/address in the profiled run; the battery run's numbers were within 1 point)
| Inclusive stage | baseline | d |
|---|---:|---:|
| `address_parser_predict` (CRF scoring incl. features) | 77.2% | 61.0% |
| `address_parser_features` (feature generation) | 14.8% | 22.2% |
| `address_parser_context_fill` (phrase/dictionary search + token normalization) | 10.8% | 18.1% |
| vocab lookups (`trie_get_data`, now only the vocab trie) | 48.2% | 7.1% |
| `char_array_add_vjoined` / `feature_array_add` (feature strings) | 5.5% / 5.9% | 8.3% / 9.0% |
| Viterbi / CRF context setup | 1.8% / 1.9% | 3.9% / 3.1% |

Exclusive time by category (d): libc string/memory ops 24.2%, CRF scoring 22.2%, trie/phrase search 20.1%, normalization/unicode 10.8%, other 6.7%, **allocator + zeroing 6.6%**, string arrays 4.6%, tokenizing 3.2%, dictionary lookup 1.5%.
Baseline for comparison: trie/phrase search 49.0%, CRF scoring 13.5%, libc string ops 12.4%, allocator 5.4%.

## Findings (all from `raw/callers-d.txt`)
- `_platform_memmove` = **11.3%** exclusive (AC rerun): callers `char_array_add_vjoined` 3.0%, **`crf_key_batch_add_token` 3.1%** (the 005/006 key copy into an arena), `crf_key_batch_lookup_dict` 1.1%, `xzm_realloc` 0.9%, `fd_split` 0.7%, `char_array_append` 0.7%.
  `_platform_strlen` 4.1% (add_vjoined 1.8%, batch add 0.9%), `_platform_memcmp` 2.1% and `memchr` 1.0% (dictionary verify/split). The key build-copy-split-verify cluster is roughly 15% of the time, part of it overhead introduced by 005/006 itself.
- `crf_tagger_score` exclusive = **13.2%, all on one source line** (`crf.c:287`, the weight-accumulate loop): cold reads of the 1.4 GB-class weight matrix (indptr -> indices/data), not arithmetic.
- **Allocation**: outermost allocator calls = 6.8% of samples inclusive (callers: `double_matrix_resize` 0.9%, `cstring_array_add_string` 0.8%, `crf_context_set_num_items` 0.6%, `char_array_cat_len` 0.5%, `cstring_array_to_strings` 0.5%, `cstring_array_new` 0.5%, ...), plus memset 1.9%
  (`crf_context_set_num_items` 1.25%: zeroing ~19 KB matrices per address). So the whole allocation+zeroing pool is about 8.7% of current time; part of it (returned response strings) cannot go away because the API returns malloc'd memory.
- Phrase search `trie_search_tokens_from_index` = 9.0% exclusive (lines trie_search.c:283/288); normalization/unicode 10.8%.

## Loaded-model memory composition (`raw/model-composition.txt`, baseline library)
| Structure | MB |
|---|---:|
| CRF state weights (CSR: 33.8M nonzeros; data 258, indices 129, indptr 44) | 430.6 |
| CRF state-trans weights (13.4M nonzeros) | 161.0 |
| CRF state-feature trie (-> dictionary 307.3 in 006) | 322.0 |
| CRF trans-feature trie (-> dictionary 47.3) | 53.9 |
| phrases trie (4.0M keys) | 115.8 |
| postal-codes trie (5.5M keys) | 105.3 |
| vocab trie (3.7M keys) | 94.6 |
| phrase types array | 15.3 |
| measured structures total | ~1,299 |
The steady footprint is ~2.4 GB, so roughly 1.1 GB is in structures not listed (the postal-code context graph, from a 621 MB file whose trie is only 105 MB, address dictionary, transliteration, buffers): not yet itemized.

## Assessment of the user's assumption (evidence, with estimates flagged)
Confirmed in direction: allocation+zeroing (~8.7% pool, realistically ~5% recoverable) is smaller than the runtime pools in key building/copying (~15%), weight-row cache misses (13%), phrase search and context fill (18%), and normalization (11%).
Memory *footprint* has more headroom than allocation *churn* does (the ~500 MB postal graph and the 1.4 GB weight arrays are unexplored), but shrinking footprint does not directly reduce runtime; it would buy room for speed structures.
Estimated achievable savings (UNVERIFIED, % of current time): fused key generation without string build/copy ~8-10; weight-row prefetch ~3-6; vocab trie -> dictionary ~5; allocation/zeroing ~5; context_fill/phrase search ~5-7; normalization ASCII fast path ~3-5.
Sum of mid-points ~33%, i.e. about 2.6-2.8x cumulative vs baseline, short of 3.5x (needs ~49% off the current time).

## Limitations
One 15 s profile per binary per condition (AC and battery agree), under load from unrelated processes; percentages are shares of a profiled run, not timings; savings above are estimates.
