# Experiment 009 — Direct key generation and weight-row prefetch (on top of 006)

Date: 2026-09-29 PDT (session 19:07-19:31). Status: **accepted (2.07x golden-v1 / 2.12x novel cumulative vs the original baseline; byte-identical; single thread; memory not increased)**
Run unattended (user away): laptop caffeinated (`caffeinate -dimsu -t 43200`), AC gate with wait-and-retry, all telemetry logged.

## Hypothesis and change
Fresh profile (008): key handling (build, copy into a batch arena, split, hash, verify) ~15% of time, of which the arena copy (`crf_key_batch_add_token`) and repeated `strlen` were overhead introduced by 005/006;
`crf.c:287` (the weight-accumulate loop) = 13% of time from cold reads of the weight matrix (indptr -> indices/data).
Changes (all in the dictionary path of `crf_tagger_score`, single thread, no cross-address state):
- **EXP_DIRECT_KEYS**: `address_parser_features` no longer clears the feature arrays per token when the CRF is accumulating (`crf_features_accumulate`); all tokens' keys stay in the same `cstring_array`s, keys are looked up straight out of them (length from string offsets, no `strlen`, no copy),
  and per-token boundaries are remembered. Accumulation order per token is unchanged, so float sums are bit-identical.
- **EXP_WEIGHT_PREFETCH**: when a lookup hits, the weight row's `indptr` entry is prefetched; a second pass prefetches `indices`/`data` of every hit row before the accumulate loop.
Variants: `e` = direct keys, `f` = direct keys + weight prefetch, `chk` = `f` plus a run-time assertion that the offset-derived length equals `strlen` for every key.

## Correctness (hard constraints)
- **Byte-identical**: `chk`, `e`, `f` match the baseline on golden-v1/all (12,863) and the novel set (12,863) (`raw/outputs_*`). The `chk` build never aborted, so the length-from-offsets assumption held on all 25,726 addresses.
- **Single thread**: no thread primitives; the controlled runs show CPU time = wall time; `ps -M` verification is folded into the final session.
- **Memory**: peak footprint below the baseline's in every process (table below). Scratch buffers only.

## Results (controlled unattended session: baseline, 006 `d`, `e`, `f`; 6 processes x 20 passes each per dataset; variants rotated; `raw/summary-quiet.md`, `raw/summary-env.md`)
| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 114 | 2.91 | nan | 100% | 36.84 | 36.47 | 36.84 | nan (0) | 36.98 |
| novel | d | 114 | 5.92 | nan | 100% | 20.24 | 20.02 | 20.23 | nan (0) | 20.29 |
| novel | e | 114 | 5.94 | nan | 100% | 18.94 | 18.78 | 18.94 | nan (0) | 19.18 |
| novel | f | 114 | 5.96 | nan | 99% | 17.39 | 17.19 | 17.38 | 16.96 (1) | 17.47 |
| v1-all | baseline | 114 | 2.92 | nan | 100% | 36.13 | 35.67 | 36.11 | nan (0) | 36.30 |
| v1-all | d | 114 | 5.93 | nan | 100% | 20.16 | 20.01 | 20.16 | nan (0) | 20.44 |
| v1-all | e | 114 | 5.95 | nan | 100% | 18.92 | 18.73 | 18.92 | nan (0) | 19.07 |
| v1-all | f | 114 | 6.02 | nan | 100% | 17.48 | 17.31 | 17.48 | nan (0) | 17.68 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | d | 1.820x | 1.822x | 1.821x | nanx | 1.823x |
| novel | e | 1.945x | 1.942x | 1.945x | nanx | 1.928x |
| novel | f | 2.119x | 2.122x | 2.119x | nanx | 2.117x |
| v1-all | d | 1.792x | 1.783x | 1.791x | nanx | 1.776x |
| v1-all | e | 1.909x | 1.904x | 1.909x | nanx | 1.903x |
| v1-all | f | 2.066x | 2.061x | 2.066x | nanx | 2.053x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   d     : countries=17 min 1.717 (lt) median 1.798 max 1.976 (ru); below 1.05x: 0
  novel   e     : countries=17 min 1.844 (lt) median 1.920 max 2.099 (ru); below 1.05x: 0
  novel   f     : countries=17 min 1.984 (lt) median 2.099 max 2.278 (ru); below 1.05x: 0
  v1-all  d     : countries=17 min 1.622 (nl) median 1.751 max 1.906 (ru); below 1.05x: 0
  v1-all  e     : countries=17 min 1.714 (nl) median 1.868 max 2.031 (ru); below 1.05x: 0
  v1-all  f     : countries=17 min 1.859 (lt) median 2.045 max 2.189 (ru); below 1.05x: 0

Peak memory footprint per process (bytes) by variant (all processes, both datasets):

| variant | processes | min | median | max | max RSS max |
|---|---:|---:|---:|---:|---:|
| baseline | 12 | 2,612,840,680 | 2,613,225,680 | 2,614,216,912 | 2,615,066,624 |
| d | 12 | 2,602,141,904 | 2,602,313,948 | 2,604,026,064 | 2,604,941,312 |
| e | 12 | 2,602,043,600 | 2,602,207,440 | 2,603,796,688 | 2,604,662,784 |
| f | 12 | 2,601,765,096 | 2,601,920,744 | 2,602,584,296 | 2,603,433,984 |

Environment during the session:
  batt: min 100.0 median 100.0 max 100.0 (n=96)
  busy_snap: min 42.5 median 49.1 max 59.0 (n=96)
  load: min 7.0 median 7.6 max 11.1 (n=96)
  swap_used_mb: min 47262.4 median 47294.4 max 47334.4 (n=96)
  battery health:           Charging: Yes;           State of Charge (%): 100;           Cycle Count: 210;           Condition: Normal;           Maximum Capacity: 95%;       Connected: Yes;       Charging: Yes; 

## Interpretation
Direct keys alone give +6.6% over `d` (1.909x vs 1.792x, golden-v1) and the weight-row prefetch adds another +8.2% (2.066x vs 1.909x): the cold reads of the weight matrix were a real memory-latency cost and prefetching hides them.
The four estimators (median, P10, CPU-time) agree within 0.5%. "Passes preempted" is ~100% because the machine is busy with unrelated processes; variants were interleaved, so comparisons are fair. The "clean pass" statistic was empty for this reason.

## Limitations
Single machine, shared and loaded; steady-state footprint and `ps -M` not re-measured for `f` in this session (done in the final verification); the direct path only applies when the dictionaries are active (otherwise the 005 path is used);
system-busy % from the during-run sampler is not captured (snapshots are).

## Decision
**Accepted** as the new best (variant `f`). Next: 010 (vocabulary trie to an exact dictionary), then allocation/zeroing cleanup and phrase search.
