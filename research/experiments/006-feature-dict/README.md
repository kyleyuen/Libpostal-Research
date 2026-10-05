# Experiment 006 — Exact compact dictionary replacing the feature tries (load-time conversion)

Date: 2026-09-29 PDT. Status: **accepted (about 1.78-1.82x over the original baseline; byte-identical; single thread; no extra memory)**

## Hypothesis and change
Bottleneck (001/002): state and prev-tag feature lookups walk a 22M-node double-array trie (about 21 dependent node loads plus data and tail loads per key). Every key has the form
`name|value...` with a small closed set of names (29 state, 2 prev-tag names *in the model file itself*), so the trie stores the name bytes (58% of key bytes) over and over.
Change: at model load (`crf_read`), enumerate each feature trie once (`feature_dict.h`), store every key as `[name id byte][rest][NUL]` in an arena plus an open-addressing table of
64-bit slots `[tag:8 | id+1:24 | offset:32]`, free the trie, and look keys up by hashing `(name id, rest)` and comparing the FULL key. Per address, all tokens' keys are batched: pass 1 splits, hashes and prefetches slots; pass 2 probes.
Lookups are exact by construction: a stored key is found iff the query equals it, which is exactly the trie's semantics (given a bijective 256-byte alphabet, checked at build).
Exactness safeguards at load: alphabet must be bijective; every leaf must be reached by the enumeration (`leaves_visited == leaves_total`, counted by a linear scan); no duplicate keys; ids < 2^24; names <= 127;
**any failure keeps the original trie** (fallback = old behaviour). Base = 005 (which sits on 003). Single thread, no cross-address state.

## Prototype gate (measurement only, before touching the library; `tools/dict_proto.c`)
Decision rule (from the advisor review): build the library version only if the dictionary is exact, at most the tries' size, and clearly faster on shuffled and novel keys.
    dict: keys=11470623 leaves_total=11470623 visited=11470623 skipped_unreachable=0 names=29 build_seconds=2.80 table=116.7 MB (15294166 slots, load 0.75) arena=190.7 MB (mean 17.43 B/key) total=307.3 MB = 95.4% of trie
    exhaustive round-trip: checked=11470623 mismatches=0 (3.6s)
    fuzz: mutations=2000000 (trie hits 300447) mismatches=0
    dict: keys=2086474 leaves_total=2086474 visited=2086474 skipped_unreachable=0 names=2 build_seconds=0.42 table=21.2 MB (2781967 slots, load 0.75) arena=26.1 MB (mean 13.10 B/key) total=47.3 MB = 87.8% of trie
    exhaustive round-trip: checked=2086474 mismatches=0 (0.5s)
    fuzz: mutations=2000000 (trie hits 402072) mismatches=0
- Exhaustive round trip and 2,000,000 fuzz mutations per trie (any-byte replace/insert, delete, truncate, break/add separator): **0 mismatches**; 0 mismatches on the dev and novel corpus keys.
- Memory: dictionaries total 354.6 MB vs 375.9 MB for the two tries (-5.7%): state 307.3 vs 322.0 MB, prev-tag 47.3 vs 53.9 MB (table 15.3M/2.8M slots, load 0.75).
  A first version with power-of-two tables was +1.3 MB larger than the tries and was rejected (`raw/dict_proto_v1_pow2.*`, kept) before switching to multiply-shift indexing.
- Lookup cost on recorded keys, ns/lookup (trie | dict | dict batched 32): dev corpus order 188 | 74 | 40; novel 192 | 78 | 42; shuffled 300 | 160-190 | n/a; prev-tag dev 145 | 73 | 34.
- Build time at load: 2.7-2.8 s (state) + 0.4 s (prev-tag).

## Library integration and correctness (hard constraints)
Variants: `dn` = dictionary only (on 003's batching + 005's dedup batch infrastructure), `d` = dictionary + all of 005 (memcpy appends, no double reset). Files: `candidate/{crf.c,crf.h,feature_dict.h,trie.c,trie.h,string_utils.c}`
(new header-only module `feature_dict.h`; `crf_t` gets two pointers appended at the END so other objects keep their layout; patches vs original in `candidate/`).
- **Byte-identical output**: `dn` and `d` match the baseline library on golden-v1/all (12,863; SHA-256 `45574ee2...c8c63`) and on the novel set (12,863; `806b8a81...6aae`) (`raw/outputs_*`).
- **Single thread**: `ps -M` showed exactly 1 thread at all 6 sample points for the baseline and for `d`; CPU seconds equal wall seconds (24.99 vs 25.07 s) (`raw/verify-*`). No thread/parallel primitives in the patch.
- **Memory (no increase)**: peak footprint per process (`time -l`, 12 processes each, controlled session): baseline 2,613,037,264-2,613,545,192 B; `d` 2,600,880,336-2,603,976,912 B (every `d` process below every baseline process).
  Steady-state footprint of the running process (`footprint -p`, same session): baseline 2,492 MB, `d` 2,481 MB (-11 MB). The baseline's steady value was 2,172 MB in 002; the difference is the machine's memory state (swap ~full), which is why all comparisons are made in the same session.
- **Setup time**: median 3.0 s -> 6.0 s (+3 s, the conversion).

## Timing
Development split, older harness (uncontrolled machine; provisional first read), 7 rounds + burn-in, 40 runs all exit 0:

| Variant | n | median addr/s | min | max | (max-min)/median | CV | median µs/addr | speedup vs baseline | vs c003 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 7 | 26,757.04 | 26,047.27 | 28,057.24 | 7.5% | 2.11% | 37.37 | 1.0000x | 0.8597x |
| c003 | 7 | 31,124.42 | 30,607.92 | 31,967.47 | 4.4% | 1.49% | 32.13 | 1.1632x | 1.0000x |
| a5 | 7 | 32,721.84 | 32,237.98 | 33,302.21 | 3.3% | 1.22% | 30.56 | 1.2229x | 1.0513x |
| dn | 7 | 45,906.67 | 44,871.43 | 47,266.52 | 5.2% | 1.65% | 21.78 | 1.7157x | 1.4749x |
| d | 7 | 49,108.37 | 46,501.98 | 50,230.48 | 7.6% | 2.44% | 20.36 | 1.8353x | 1.5778x |

Controlled session (see 007; interference-aware, all 12,863 golden-v1 addresses and the novel set): `d` = **1.778x** (golden-v1), **1.800x** (novel) vs baseline by median; P10, CPU-time and clean-pass estimators agree within ~2%;
cold first pass 1.786x / 1.735x; per-country speedup min 1.617x (lt), median 1.73x, max 1.90x (ru) on golden-v1 and min 1.654x / median 1.757x / max 1.930x on novel; no country below 1.05x.
`dn` (dictionary alone, without 005): 1.682x / 1.737x.
Validation split alone (frozen `parser_profile` harness, AC gate on, 7 rounds + burn-in, alternating):

```
validation split (2,555 addresses), frozen parser_profile harness, AC power gate on, 7 rounds + burn-in, alternating order
baseline  median 27,826 addr/s (35.94 us/addr)  min 27,643 max 28,383  (max-min)/median 2.7%
d         median 50,558 addr/s (19.78 us/addr)  min 50,259 max 51,116  (max-min)/median 1.7%
speedup by medians 1.817x ; paired median 1.816x (min 1.798 max 1.836); d faster in 7/7 rounds
```

## Limitations and remaining uncertainty
- The dictionary is built from one model (default v1.0.0 files) whose trie alphabet is the full 256-byte permutation; a different model with another alphabet falls back to the trie automatically (untested here).
- Setup time roughly doubles (+3 s). If the hidden evaluation times a single short run including setup, that overhead would count against us; throughput-style evaluation is favored.
- Only the state and prev-tag feature tries are converted. Vocabulary lookups (`word_vocab_frequency`), phrase search, normalization and feature-string building are unchanged and now form most of the remaining time.
- Machine was shared and loaded (see 007 environment log); 81-89% of passes saw an involuntary context switch. Estimators that discount interference agree, but absolute numbers are for this laptop only.
- Steady footprint was sampled once per binary in one session (plus peak footprint over 12 processes per variant).
- `CRF_DICT_DISABLE` / `CRF_DICT_VERBOSE` environment switches exist in the experiment builds for A-B testing; they would be removed from a production patch.

## Decision
**Accepted.** Combined with 003 and 005 this is the first result near a 2x improvement on this machine: **1.78x on golden-v1, 1.80x on the novel set, 1.82x on validation**, byte-identical, single thread, memory not increased.
The original 3.5x target is not yet reached. Remaining time per address is about 20 us: feature generation, phrase search (`trie_search_tokens_from_index`), normalization, weight accumulation and vocab lookups.
Next candidates: apply the same exact-dictionary idea to the vocab trie (`word_vocab_frequency`, called up to 3x per word), cache per-token vocab results, optimize `context_fill`'s phrase search, and re-profile.
Ledger, report and activity log updated: yes.
