# Diagnostic hook plan — 194 length-aware trie

Source-only derivative of the immutable 194 candidate over prototype181's 185 hooks. No build or parser run by implementation author.

Metrics: `length_aware_lookup_calls` counts trie_get_data_len invocations with non-null keys; `key_strlen_scan_bytes_avoided` sums caller-supplied key lengths as a work proxy for the former per-query strlen traversal (not CPU cycles); `compressed_tail_compare_bytes` sums requested strncmp bounds at compressed-tail comparisons (upper-bound bytes, not exact bytes read); standard `diag_lookup` records lookup calls/hits/misses by dict, operation, stage, family. Legacy APIs still use the diagnostic strlen-based lookup wrapper.
