# Experiment 049 — warm-only profile of 048 `va` (diagnostic)

Date/time/timezone: 2026-10-02 ~18:50 PDT (`raw/`).
Status: complete (measurement only)

`sample` (15 s, 1 ms) of `va` bench_ext cycling the first 30 golden-v1 development addresses (400,000 passes) so that model data stays cached and the profile shows compute only. First attempt ended before sampling (40,000 passes too short; `raw/attempt1_too_short/`). Summary `raw/profile_report.stdout`, callers `raw/callers.stdout`.
Top exclusive: `address_parser_parse` (LTO-inlined) 34.1%, `crf_dict_lookup_array` 9.0%, `memmove` 6.0% (cstring_array_add_string 1.4, normalize_token 0.85, feature_array_add 0.75, …), `trie_search_tokens_from_index` 5.7%, `memset` 5.7% (all inlined into parse: per-address score-buffer clearing), `strlen` 4.2% + stub 1.1% (feature_array_add, feature_array_add_named, …), `normalize_token` 3.9%, `feature_array_add_named` 3.2%, `address_parser_features` 3.0%, `ap_ik_emit1` 3.0%, `memchr` 1.4% and `memcmp` 1.3% (lookup key split/compare). libc string/memory routines ≈21% of warm samples. Harness timer calls ≈2.8%.
