# Golden-v1 baseline results

Measured September 28, 2026; same default model, Apple M4 Pro and native arm64 library build as the initial experiment. No optimization applied.

| Run | Addresses/s | µs/address | Wall seconds | CPU seconds | Peak RSS bytes |
|---|---:|---:|---:|---:|---:|
| 1 | 27542.83 | 36.31 | 10.274398 | 10.241350 | 2613886976 |
| 2 | 28602.47 | 34.96 | 10.343478 | 10.340905 | 2613690368 |
| 3 | 27821.28 | 35.94 | 10.171566 | 10.157674 | 2613690368 |

**Median: 27,821.28 addresses/s; peak RSS: 2.434 GiB.**
A 3.5x result on this setup would be approximately 97,374 addresses/s.

The 12,863 inputs are visited in fixed SHA-256 order, after a complete warm-up traversal. The timer ends after a complete corpus pass, so nominal 10-second trials run slightly longer. Timings are independent of the profiler. Corpus-size/cache effects make comparisons to the old 70-input corpus unsuitable for claiming a speedup.

A separate 25-second run was sampled for 15 seconds with a requested 1 ms interval: 12,743 samples. Inclusive percentages overlap and must not be added.

| Stage | Inclusive sampled share |
|---|---:|
| `crf_tagger_score` | 74.2% |
| `trie_get_data` | 48.7% |
| `address_parser_features` | 15.3% |
| `address_parser_context_fill` | 10.1% |
| `normalize_string_latin` | 7.6% |
| `char_array_add_vjoined` | 6.6% |
| `crf_context_viterbi` | 2.3% |
| `scan_token` | 0.9% |

Trie lookup is now about half of sampled time, making feature lookup the clearest initial target. The difference from the small-corpus profile is consistent with changed input mix and cache behavior, but no hardware-counter measurements were taken, so cache misses are not established as the cause.

Even eliminating trie lookup completely would only provide roughly 1.95x if everything else stayed fixed. Thus 3.5x still requires improvements beyond trie lookup. These are baseline observations, not an achieved optimization.

Validation: every baseline output was regenerated and compared, 12,863 checked with zero mismatches. The checker also rejected a deliberately changed component with exactly one mismatch. Rebuilding the dataset produced byte-identical files. The C harness successfully handled all 12,863 inputs on each run. Supplied annotation accuracy was not evaluated.
