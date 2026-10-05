# Experiment 012 — Inline dictionary hash/compare, restructured Viterbi, phrase-search prefetch (on top of 011 `h`)

Date: 2026-09-29 PDT (session 21:21-21:48). Status: **rejected — all three changes are slower than the control; none is carried forward. `h` (011) remains the best candidate.**
Run unattended (caffeinated, AC gate).

## Hypotheses (from a fresh profile of `h`, `008-profile-after-006/raw/profile-h.txt`, AC power)
- `_platform_memmove` 7.3% and `_platform_memcmp` 3.4%: my own dictionary code called libc for variable-length tail loads (`memcpy(&w, s, len)`) and for a ~13-byte key compare -> **iA** (`EXP_FD_INLINE`): inline overlapping fixed-size loads for the hash tail and the equality test.
- `crf_context_viterbi` 5.2% with two accessor calls per (i, j) in the innermost loop -> **iB** (`EXP_FAST_VITERBI`): source label outer, target label inner, hoisted row pointers, same strict-greater tie-breaking and the same float association `(prev[i] + st[j]) + tr[j]`.
- `trie_search_tokens_from_index` 12.6% exclusive, independent per-token walks that leave the trie after 1-2 characters -> **iC** (`EXP_SEARCH_PREFETCH`): prefetch every token's second-level node (or leaf data entry) before the walk (a pure hint).
Control `h0` = 011 `h` rebuilt from the 012 sources with no new switch; `i` = all three.

## Correctness (all fine; they were just slower)
- **Byte-identical**: `h0`, `iA`, `iB`, `iC`, `i` match the baseline on golden-v1/all and the novel set (12,863 each).
- **Dictionary exactness re-verified for the inline hash** (`tools` of 006/010 recompiled with `-DEXP_FD_INLINE`): feature tries 11,470,623 + 2,086,474 keys and the vocabulary 3,694,560 keys round-trip with 0 mismatches; 2,000,000 fuzz mutations per structure, 0 mismatches (`raw/dict_proto_inline.stdout`, `raw/vocab_proto_inline.stdout`).

## Results (controlled unattended session; baseline, 011 `h`, `h0`, `iA`, `iB`, `iC`, `i`; 5 processes x 19 timed passes per variant per dataset)
| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 95 | 2.91 | nan | 100% | 35.64 | 35.22 | 35.62 | nan (0) | 35.76 |
| novel | h | 95 | 6.58 | nan | 100% | 13.27 | 13.16 | 13.26 | nan (0) | 13.45 |
| novel | h0 | 95 | 6.59 | nan | 100% | 13.26 | 13.18 | 13.26 | nan (0) | 13.33 |
| novel | iA | 95 | 6.49 | nan | 100% | 13.38 | 13.28 | 13.38 | nan (0) | 13.64 |
| novel | iB | 95 | 6.53 | nan | 100% | 13.71 | 13.60 | 13.70 | nan (0) | 13.84 |
| novel | iC | 95 | 6.57 | nan | 100% | 13.48 | 13.39 | 13.47 | nan (0) | 13.70 |
| novel | i | 95 | 6.51 | nan | 100% | 13.62 | 13.51 | 13.62 | nan (0) | 13.76 |
| v1-all | baseline | 95 | 2.91 | nan | 100% | 35.14 | 34.69 | 35.13 | nan (0) | 35.31 |
| v1-all | h | 95 | 6.59 | nan | 100% | 13.25 | 13.15 | 13.25 | nan (0) | 13.43 |
| v1-all | h0 | 95 | 6.55 | nan | 100% | 13.23 | 13.11 | 13.23 | nan (0) | 13.41 |
| v1-all | iA | 95 | 6.47 | nan | 100% | 13.33 | 13.21 | 13.33 | nan (0) | 13.53 |
| v1-all | iB | 95 | 6.57 | nan | 100% | 13.60 | 13.46 | 13.59 | nan (0) | 13.79 |
| v1-all | iC | 95 | 6.57 | nan | 100% | 13.43 | 13.28 | 13.42 | nan (0) | 13.59 |
| v1-all | i | 95 | 6.47 | nan | 100% | 13.49 | 13.38 | 13.48 | nan (0) | 13.65 |


Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | h | 2.686x | 2.676x | 2.686x | nanx | 2.660x |
| novel | h0 | 2.688x | 2.673x | 2.687x | nanx | 2.682x |
| novel | iA | 2.663x | 2.653x | 2.663x | nanx | 2.622x |
| novel | iB | 2.600x | 2.589x | 2.599x | nanx | 2.585x |
| novel | iC | 2.644x | 2.630x | 2.644x | nanx | 2.611x |
| novel | i | 2.617x | 2.606x | 2.616x | nanx | 2.599x |
| v1-all | h | 2.651x | 2.637x | 2.652x | nanx | 2.629x |
| v1-all | h0 | 2.655x | 2.647x | 2.655x | nanx | 2.633x |
| v1-all | iA | 2.636x | 2.627x | 2.636x | nanx | 2.610x |
| v1-all | iB | 2.584x | 2.577x | 2.585x | nanx | 2.560x |
| v1-all | iC | 2.617x | 2.613x | 2.617x | nanx | 2.598x |
| v1-all | i | 2.605x | 2.594x | 2.605x | nanx | 2.586x |



Peak memory footprint per process (bytes) by variant (all processes, both datasets):

| variant | processes | min | median | max | max RSS max |
|---|---:|---:|---:|---:|---:|
| baseline | 10 | 2,613,037,264 | 2,613,233,896 | 2,614,347,984 | 2,615,197,696 |
| h | 10 | 2,557,020,320 | 2,557,331,616 | 2,559,084,704 | 2,559,934,464 |
| h0 | 10 | 2,557,020,320 | 2,557,249,732 | 2,557,905,056 | 2,558,754,816 |
| iA | 10 | 2,557,135,032 | 2,557,446,304 | 2,558,625,952 | 2,559,475,712 |
| iB | 10 | 2,557,151,392 | 2,557,356,204 | 2,558,494,904 | 2,559,328,256 |
| iC | 10 | 2,557,102,264 | 2,557,282,476 | 2,557,462,712 | 2,558,263,296 |
| i | 10 | 2,557,118,624 | 2,557,364,396 | 2,558,544,008 | 2,559,459,328 |



Relative to the control `h0` (golden-v1 / novel, by median): `iA` -0.7% / -0.9%, `iB` -2.7% / -3.3%, `iC` -1.4% / -1.6%, `i` -1.9% / -2.6%. The control `h0` is within 0.2% of `h` (rebuild noise), so the deficits are real and consistent across both datasets and all estimators.

## Interpretation (unverified hypotheses; no counters available)
- My estimates for all three (up to ~12% combined) were wrong in sign. At `-O2` the compiler already handles the libc calls well enough that my extra branches on unpredictable sizes cost about as much as they save (iA). The original Viterbi loop nest evidently maps better to the hardware than my
  swapped version, which adds a best/arg array round trip per source label (iB). The search walks were already short and hot, so the extra pass and its loads cost more than they hide (iC).
- Lesson recorded: a profile share is an upper bound on what can be saved, not an estimate of what a rewrite gains; three of my last four micro-changes came in at or below zero.

## Decision
**Rejected.** Retained as a documented failed attempt (patches in `candidate/*.patch`, all raw data). Best candidate remains 011 `h`. Ledger, report and activity log updated: yes.
