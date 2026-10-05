# Experiment 045 — memory touched per address, by structure (trace build) and translation-miss cost (diagnostic)

Date/time/timezone: created 2026-10-02T17:18:40-0700; runs ~17:50–18:05 PDT (`raw/run.log`), America/Los_Angeles.
Status: complete (measurement only)

## Method
Switch `EXP_TRACE` (experiment copy of `l3` sources; new header `sources/trace_touch.h`; runtime in `address_parser.c`) records every read of: feature-dictionary slots and key arena, integer-key slots, row index, state and transition weight rows, role-id table, vocabulary slots and arena, trie nodes/data/tails (component, postal, other), postcode-context graph (probes replicated from `graph_has_edge`). Per address: distinct 128-byte lines and 16 KB pages per structure, lines new to the run, misses of fully associative LRU caches (16 MiB, 48 MiB) and page-LRU caches (192 / 3,072 / 12,288 pages) fed with each address's distinct lines in first-touch order. -O2, no PGO/LTO; golden-v1 development. Attempt 1 counted the model-load role-table build as the first address (`raw/attempt1_load_counted/`); fixed by discarding the pre-first-parse block. Address-dictionary expansion arrays, tokenizer tables and code are not traced.

## Results (per address; `raw/trace_lines.stderr`, `raw/trace_pages.stderr`)

    addresses=10308 (per address: distinct_lines distinct_pages new_in_run lru16MiB_miss lru48MiB_miss | page-LRU misses at 192 / 3072 / 12288 pages)
    feat_slots         28.48    27.94    12.03    14.84    12.44 |    26.93    20.39    14.65
    feat_arena         23.86    22.27     7.76    10.18     8.08 |    21.27    15.00    10.36
    int_key_slots       0.00     0.00     0.00     0.00     0.00 |     0.00     0.00     0.00
    row_index          23.50    14.71     5.32     7.61     5.60 |    12.54     6.50     3.17
    state_rows         50.35    13.75     9.22    12.53     9.63 |    12.37     8.48     6.13
    trans_rows         55.37     6.75     5.29     7.43     5.54 |     5.75     3.48     2.50
    role_ids            5.67     4.86     1.57     1.97     1.62 |     4.46     2.78     2.22
    vocab_slots         6.54     6.14     1.97     2.55     2.04 |     5.72     3.81     2.76
    vocab_arena         6.05     5.41     1.61     2.11     1.67 |     4.74     2.98     2.14
    trie_component     39.97    23.62     8.19    11.93     8.62 |    21.02    10.51     5.90
    trie_postal        23.98    19.20     2.80     4.13     2.96 |    16.74     7.18     2.66
    trie_other         40.31    20.15     0.73     1.44     0.76 |    12.87     0.77     0.20
    postcode_ctx        3.93     2.56     1.97     2.54     2.06 |     2.51     1.84     1.29
    TOTAL             308.01   167.36    58.47    79.26    61.03 |   146.92    83.73    53.98

Translation-miss microbenchmark (`tools/tlb_probe.c`, `raw/tlb_probe.stdout`): dependent chase over 8,192 cached lines, packed vs one line per 16 KB page over 2 GB: 6.3 vs 16.7 ns per access (≈+10 ns); with 8 independent chains 1.35 vs 2.92 ns (≈+1.6 ns). Superpages are unavailable (041).

## Interpretation
308 distinct lines and 167 pages per address; only 58 lines are new to the run and 79 miss a 16 MiB LRU, so most first-touch cost is lines evicted from the private caches (shared-cache contention on a host with load average ~8 is likely; not controllable). Translation misses add little when data is cached. Combined with 037/046, per-address memory cost scales with the number of distinct lines touched rather than with exposed latency.

## Extension v4 (expansion data and phrase types; `raw/trace_v4.stderr`)
Added address-dictionary expansion values and arrays (all eight `address_dictionary_get_expansions` call sites and the expansion elements read by `is_valid_dictionary_phrase`) and `parser->phrase_types`. Per address they add 7.2 distinct lines and 1.5 misses of a 16 MiB LRU (expansion value 1.65 / 0.20, expansion array 2.92 / 0.40, phrase types 2.62 / 0.94). Total: 315 distinct lines, 174 pages, 59.5 new-in-run, 81 LRU-16 MiB misses per address.

Cost per modelled miss (043 first-touch µs ÷ 16 MiB-LRU misses) remains inconsistent across paths: lookups+accumulation ≈25 ns (batched with prefetch), component trie ≈75 ns, postal trie ≈97 ns, feature generation ≈120 ns, address dictionary + prefix/suffix ≈230 ns. Interpretation (UNVERIFIED): dependent-chain paths pay close to full memory latency per miss, and real misses exceed the private fully associative model because the 16 MB L2 is shared with co-running processes (load average 7–10 during the session, two processes at ~170% CPU). The trace does not cover code, tokenizer tables or allocator metadata.
