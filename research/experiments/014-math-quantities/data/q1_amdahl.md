## Q1a. Savings needed (6.4), recomputed: saved = T_now - T_base/S

| Target S | Full: saved us | Full % of T_now | Floor: saved us | Floor % of T_now |
|---:|---:|---:|---:|---:|
| 3.5x | 3.25 | 24.6% | 4.91 | 33.5% |
| 4x | 4.50 | 34.0% | 6.13 | 41.8% |
| 5x | 6.24 | 47.2% | 7.84 | 53.4% |
| 7x | 8.24 | 62.3% | 9.79 | 66.7% |

## Q1b. Per-item ceiling p*T_now (upper bound, not an estimate of the gain)

| Item | Stage | p % | Ceiling full us | Ceiling floor us* | Speedup if ceiling fully removed (full) | Basis for p |
|---|---|---:|---:|---:|---:|---|
| N1 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | stage: normalization (no function share quoted) |
| N2 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | stage: normalization |
| N3 | whole-string normalization (~10%) | 1.4 | 0.19 | 0.21 | 2.68x | trie_get_prefix_from_index 1.2-1.4% |
| N4 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | stage: normalization |
| N5 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | stage: normalization |
| N6 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | part of N5 (not additive) |
| N7 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | stage: normalization |
| N8 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | stage: normalization |
| N9 | whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | 2.93x | stage: normalization |
| N10 | tokenizing | 8.1 | 1.07 | 1.19 | 2.87x | stages: tokenizing + allocation |
| N11 | tokenizing | 2.4 | 0.32 | 0.35 | 2.70x | stage: tokenizing loop (prev2 clear is in feature gen) |
| N12 | tokenizing | 2.4 | 0.32 | 0.35 | 2.70x | tokenizing 2.4% inclusive |
| N13 | tokenizing | 3.7 | 0.49 | 0.54 | 2.74x | _platform_strlen 3.7% (all callers) |
| P1 | context_fill | 12.6 | 1.67 | 1.85 | 3.02x | trie_search_tokens_from_index 12.6% own |
| P2 | context_fill | 3.0 | 0.40 | 0.44 | 2.72x | prefix/suffix searches ~3% inclusive |
| P3 | context_fill | 4.3 | 0.57 | 0.63 | 2.76x | normalize_token 4.3% inclusive |
| P4 | context_fill | 4.3 | 0.57 | 0.63 | 2.76x | normalize_token 4.3% inclusive |
| P5 | context_fill | 24.0 | 3.17 | 3.52 | 3.47x | stage: context_fill |
| P6 | context_fill | 24.0 | 3.17 | 3.52 | 3.47x | stage: context_fill |
| P7 | context_fill | 24.0 | 3.17 | 3.52 | 3.47x | is_valid_dictionary_phrase 0.3% + unquoted callee; stage cap |
| P8 | context_fill | 2.1 | 0.28 | 0.31 | 2.70x | graph_has_edge 2.1% own |
| P9 | context_fill | 12.6 | 1.67 | 1.85 | 3.02x | same searches as P1 |
| P10 | context_fill | 15.0 | 1.98 | 2.20 | 3.10x | phrase search ~15% inclusive |
| P11 | context_fill | 24.0 | 3.17 | 3.52 | 3.47x | stage: context_fill |
| P12 | context_fill | 1.1 | 0.15 | 0.16 | 2.67x | _platform_strncmp 1.0-1.1% |
| F1 | feature generation | 19.0 | 2.51 | 2.79 | 3.26x | stage: feature generation (vocab lookup 3.7% is part) |
| F2 | feature generation | 34.0 | 4.49 | 4.99 | 4.00x | stages: feature gen + duplicate lookups |
| F3 | dictionary lookups (12-15%) | 23.7 | 3.13 | 3.48 | 3.46x | stages: lookup + accumulate of 'bias' |
| F4 | dictionary lookups (12-15%) | 15.0 | 1.98 | 2.20 | 3.10x | stage: dictionary lookups |
| F5 | dictionary lookups (12-15%) | 7.3 | 0.97 | 1.07 | 2.85x | fd_split 2.5% + memchr 1.4% + memcmp 3.4% |
| F6 | dictionary lookups (12-15%) | 2.5 | 0.33 | 0.37 | 2.71x | fd_split 2.5% (contains fd_find_name) |
| F7 | feature generation | 19.0 | 2.51 | 2.79 | 3.26x | stage: feature generation |
| F8 | feature generation | 19.0 | 2.51 | 2.79 | 3.26x | stage: feature generation |
| F9 | feature generation | 19.0 | 2.51 | 2.79 | 3.26x | covered by F1/F2 |
| F10 | feature generation | 19.0 | 2.51 | 2.79 | 3.26x | stage: feature generation |
| F11 | dictionary lookups (12-15%) | 15.0 | 1.98 | 2.20 | 3.10x | stage: dictionary lookups |
| F12 | feature generation | 3.7 | 0.49 | 0.54 | 2.74x | _platform_strlen 3.7% (all callers) |
| C1 | dictionary lookups (12-15%) | 23.7 | 3.13 | 3.48 | 3.46x | stages: lookup + accumulate |
| C2 | weight accumulation + zeroing | 1.8 | 0.24 | 0.26 | 2.69x | memset 1.8% own |
| C3 | weight accumulation + zeroing | 8.7 | 1.15 | 1.28 | 2.89x | stage: accumulate/zeroing |
| C4 | Viterbi | 5.2 | 0.69 | 0.76 | 2.78x | Viterbi 5.2% |
| C5 | Viterbi | 5.2 | 0.69 | 0.76 | 2.78x | Viterbi 5.2% |
| C6 | response build/free (~3%) | 3.0 | 0.40 | 0.44 | 2.72x | stage: response |
| C7 | dictionary lookups (12-15%) | 15.0 | 1.98 | 2.20 | 3.10x | stage: dictionary lookups |
| C8 | weight accumulation + zeroing | 8.7 | 1.15 | 1.28 | 2.89x | stage: accumulate |
| C9 | weight accumulation + zeroing | 8.7 | 1.15 | 1.28 | 2.89x | stage: accumulate |
| C10 | weight accumulation + zeroing | 8.7 | 1.15 | 1.28 | 2.89x | reads weights; audit expects speed-neutral |
| C11 | weight accumulation + zeroing | 8.7 | 1.15 | 1.28 | 2.89x | stage: accumulate |
| R1 | response build/free (~3%) | 4.4 | 0.58 | 0.65 | 2.76x | cstring_array_to_strings 1.8 + destroy 0.6 + allocations ~2 |
| R2 | allocation and freeing | 8.0 | 1.06 | 1.17 | 2.87x | allocator 5.7% inclusive + zeroing 2.3% |
| R3 | allocation and freeing | 1.2 | 0.16 | 0.18 | 2.67x | xzm_realloc + memmove 1.2% own |
| R4 | response build/free (~3%) | 3.0 | 0.40 | 0.44 | 2.72x | covered by C6 |
| R5 | allocation and freeing | 8.0 | 1.06 | 1.17 | 2.87x | covered by R2 |
| R6 | response build/free (~3%) | 3.0 | 0.40 | 0.44 | 2.72x | stage: response |
| R7 | response build/free (~3%) | 3.0 | 0.40 | 0.44 | 2.72x | stage: response |
| L1 | load time (not in parse time) | 0.0 | 0.00 | 0.00 | 2.64x | memory item; lookup cost covered by P8 |
| L2 | load time (not in parse time) | 0.0 | 0.00 | 0.00 | 2.64x | load time only |
| L3 | load time (not in parse time) | 0.0 | 0.00 | 0.00 | 2.64x | load time only |
| L4 | dictionary lookups (12-15%) | 23.7 | 3.13 | 3.48 | 3.46x | layout of lookup + weight tables |
| L5 | load time (not in parse time) | 0.0 | 0.00 | 0.00 | 2.64x | cold pass only |
| L6 | load time (not in parse time) | 0.0 | 0.00 | 0.00 | 2.64x | load time only |
| X1 | whole program | 100.0 | 13.22 | 14.67 | unbounded | whole program |
| X2 | whole program | 12.9 | 1.71 | 1.89 | 3.03x | stubs 0.8+1.1 + memmove 7.3 + strlen 3.7 |
| X3 | whole program | 100.0 | 13.22 | 14.67 | unbounded | whole program (subsumed by X1) |
| X4 | whole program | 100.0 | 13.22 | 14.67 | unbounded | whole program |
| X5 | feature generation | 8.0 | 1.06 | 1.17 | 2.87x | feature_array_add 8.0% inclusive |
| 3.1 | weight accumulation + zeroing | 42.7 | 5.64 | 6.26 | 4.60x | U-block keys: generation + lookup + accumulate |
| 3.2 | dictionary lookups (12-15%) | 34.0 | 4.49 | 4.99 | 4.00x | key building + lookup |

## Q1c. Per stage: stage ceiling and naive sum of item ceilings (overlapping; the stage ceiling is the binding bound)

| Stage | Stage share % | Stage ceiling full us | Stage ceiling floor us* | Items | Sum of item ceilings full us (overlaps) |
|---|---:|---:|---:|---|---:|
| context_fill | 24.0 | 3.17 | 3.52 | P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12 | 19.96 |
| feature generation | 19.0 | 2.51 | 2.79 | F1, F2, F7, F8, F9, F10, F12, X5 | 18.60 |
| dictionary lookups (12-15%) | 15.0 | 1.98 | 2.20 | F3, F4, F5, F6, F11, C1, C7, L4, 3.2 | 21.14 |
| weight accumulation + zeroing | 8.7 | 1.15 | 1.28 | C2, C3, C8, C9, C10, C11, 3.1 | 11.63 |
| Viterbi | 5.2 | 0.69 | 0.76 | C4, C5 | 1.37 |
| allocation and freeing | 5.7 | 0.75 | 0.84 | R2, R3, R5 | 2.27 |
| whole-string normalization (~10%) | 10.0 | 1.32 | 1.47 | N1, N2, N3, N4, N5, N6, N7, N8, N9 | 10.76 |
| tokenizing | 2.4 | 0.32 | 0.35 | N10, N11, N12, N13 | 2.19 |
| response build/free (~3%) | 3.0 | 0.40 | 0.44 | C6, R1, R4, R6, R7 | 2.17 |
| load time (not in parse time) | 0.0 | 0.00 | 0.00 | L1, L2, L3, L5, L6 | 0.00 |
| whole program | 100.0 | 13.22 | 14.67 | X1, X2, X3, X4 | 41.37 |

Sum of the nine parse-path stage shares: 93.0% (shares are approximate and inclusive; the gap to 100% is profile overlap/omission).
- 3.5x on the full set needs 24.6% of T_now removed; floor set 33.5%.
- 4x on the full set needs 34.0% of T_now removed; floor set 41.8%.
- 5x on the full set needs 47.2% of T_now removed; floor set 53.4%.
- 7x on the full set needs 62.3% of T_now removed; floor set 66.7%.

*Floor-set column applies the full-set profile shares to 14.67 us: ASSUMPTION (no floor-set profile exists).
