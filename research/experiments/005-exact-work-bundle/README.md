# Experiment 005 — Exact work-elimination bundle (structural, corpus-independent)

Date: 2026-09-29 PDT (development timing 13:22-13:36 and powered run 13:38-13:51). Status: **small verified increment (+4.4% over 003, byte-identical); carried into 006, milestone validation folded into 006**

## Why (design rules from METHOD.md "Generalization protocol")
All changes remove work the code does redundantly. None depend on input statistics, none cache results across addresses, none change output.
Findings from 001/002 and the source (not from tuning): (1) `char_array_append`/`_append_len` push one byte at a time with a bounds check per byte, and are the
inner loop of `char_array_add_vjoined` (feature strings), context_fill and normalization; (2) `crf_tagger_score` zeroes the same matrices twice
(`set_num_items` then `crf_context_reset(ALL)`), zeroes `trans` right before overwriting it with `trans_weights`, and zeroes unused marginal arrays because the
parser's context is created with the marginals flag; Viterbi reads only `state`, `state_trans`, `trans` (+ scratch it overwrites); (3) 12.7-13.0% of state-feature
lookups per address are exact duplicates (phrase features repeat across the tokens of a phrase; same rate on development and validation), and each lookup calls `strlen`
several times although the length is known.

## Changes (each switchable at compile time for ablation)
- **A `EXP_FAST_APPEND`** (`string_utils.c`): `char_array_append` / `char_array_append_len` reserve once (same doubling policy as push) and `memcpy`. Same bytes.
- **B `EXP_NO_RESET`** (`crf.c`): drop the redundant `crf_context_reset(ALL)` in `crf_tagger_score` (state/state_trans already zeroed by `set_num_items`; `trans` overwritten by the copy; marginals unused).
- **C `EXP_DEDUP_BATCH`** (`crf.c`, `trie.c`): compute all tokens' features first, look up each *distinct* key of the address once using its known length
  (`trie_get_data_len`, no `strlen`), then accumulate weights per token in the original order (identical float operations). Per-address scratch buffers only; no cross-address state.
Base: 003 (crf batching). Variants: `a`, `b`, `c` (each on top of 003) and `all`. Single thread, byte-identical required (dump on golden-v1 all + novel).

## Plan
build a/b/c/all; identity on golden-v1 all and golden-v2-novel; development timing (baseline, c003, a, b, c, all; rotated; 7 rounds + burn-in); pick on development;
then validation, all, novel-set, cold-pass and per-country checks for the chosen; steady footprint + `ps -M`; write-up.

## Results

Correctness (hard constraint): the five variants (n control, a, b, c, all) are **byte-identical** to the baseline on golden-v1/all (12,863; SHA-256 `45574ee2…c8c63`) **and** on the novel set
(12,863; SHA-256 `806b8a81…6aae`): 25,726 outputs each, `raw/outputs_*`. Single thread: no thread primitives in the patches; per-address scratch only (a static buffer, like 003).
Memory: only reusable scratch buffers (a few KB) are added; steady-state footprint and the `ps -M` check are folded into 006's milestone.

### Development ablation (baseline, 003, control `n`, a, b, c, all; 7 rounds + burn-in; 56 runs all exit 0; `raw/bench-development-summary.md`)

| Variant | n | median addr/s | min | max | (max-min)/median | CV | median µs/addr | speedup vs baseline | vs c003 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 7 | 29,566.44 | 26,307.43 | 30,142.29 | 13.0% | 4.36% | 33.82 | 1.0000x | 0.8634x |
| c003 | 7 | 34,243.00 | 32,852.73 | 34,630.28 | 5.2% | 2.01% | 29.20 | 1.1582x | 1.0000x |
| n | 7 | 32,612.04 | 30,840.60 | 34,277.23 | 10.5% | 3.51% | 30.66 | 1.1030x | 0.9524x |
| a | 7 | 33,988.98 | 33,155.41 | 35,343.41 | 6.4% | 2.41% | 29.42 | 1.1496x | 0.9926x |
| b | 7 | 33,713.97 | 32,410.31 | 35,327.63 | 8.7% | 3.10% | 29.66 | 1.1403x | 0.9846x |
| c | 7 | 33,867.71 | 28,747.81 | 34,254.97 | 16.3% | 5.47% | 29.53 | 1.1455x | 0.9890x |
| all | 7 | 34,560.95 | 33,501.85 | 35,844.74 | 6.8% | 2.39% | 28.93 | 1.1689x | 1.0093x |

Median paired ratios vs 003: n 0.977, a 1.010, b 0.990, c 0.997, all 1.029 (range 0.976-1.047). Every single part was within +-1% (inside the ~5% noise band; the control `n`, which is 003's logic recompiled, sat at 0.977, showing the rebuild noise), and the bundle was inconclusive on 7 rounds.
My source-reading estimate (10-14%) was too optimistic: the redundant zeroing, per-byte pushes and duplicate lookups are real but mostly hidden by the CPU.

### Powered head-to-head (development; c003 vs all; 20 alternating rounds after a burn-in; `raw/powered-summary.txt`)
c003 median 33,226 vs all 34,613 addr/s: **ratio 1.042** (medians), **paired median 1.044**, better in **18 of 20** rounds, middle half 1.035-1.066 (min 0.987, max 1.099).
So the bundle is a small but real +4.4% over 003 (about 1.20x vs baseline on this split).

## Interpretation and limitations
- The gain is small and was only resolved with 20 paired rounds; the 5% "inconclusive" rule from 002 was applied strictly to the 7-round run and then relaxed because a powered paired test resolved it. Not attributed to a single part; parts are individually below noise.
- Not run yet for this change alone: validation/all/novel timing, cold pass, per-country, steady footprint, `ps -M`. They will be run on the final candidate in 006 (which includes this bundle), under the generalization protocol.
- No corpus-derived tables or cross-address caches were used (design rule).

## Reproduction
`tools/build.sh` (variants n/a/b/c/all), `tools/run_bench.sh`, `tools/run_bench_tag.sh` (powered), `tools/analyze_bench.py`; patches in `candidate/*.patch`; hashes in `raw/hashes.txt` and `artifacts/*/hashes.txt`.

## Decision
**Kept as a small verified increment** (+4.4% over 003, 18/20 paired rounds, byte-identical). Adopted as the base of 006. Ledger, report and activity log updated: yes.
