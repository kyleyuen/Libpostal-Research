# Experiment 003 — Batch feature lookups before accumulating weights (`crf_tagger_score`)

Date: 2026-09-29 PDT (development timing 12:25-12:33, validation/all 12:34-12:51). Status: **accepted (incremental gain ~1.14x; original 3.5x target not met)**

## Hypothesis and change
Bottleneck (measured in 001/002): state and prev-tag feature lookup+accumulate is 56.6% of time; a lookup costs ~245 ns in the pipeline vs ~56 ns
cache-resident. In the library's order each feature does {trie walk, then dependent weight accumulation} before the next trie walk starts.
Hypothesis: doing all trie walks for a token first (storing feature ids), then accumulating in the same order, lets the CPU overlap cache-missing
loads across independent walks. 001's harness replica measured ~12.7% faster (30.7 vs 35.2 us/addr), mechanism unverified.
Exact change: `crf_tagger_score` in `crf.c` only (see `candidate/crf.patch`). Accumulation order and values are unchanged, so outputs must be bit-identical.
Base: original libpostal commit `25099c50…`, `src/crf.c` sha256 in `raw/hashes.txt`. Incremental base = original (first code change).
Only `libpostal_la-crf.o` is replaced inside a copy of the preserved baseline `libpostal.a`; everything else is byte-identical to baseline.
A control build (unchanged crf.c recompiled the same way) checks that the rebuild pipeline itself does not change speed.

## Plan (protocol from 002)
1. Build control + candidate; hash. 2. Exact-output check on all 12,863 (candidate and control) via `check_outputs.py --library` (needs a dylib) AND a
C-level ordered comparison from the static build. 3. Development-split throughput, alternating baseline/control/candidate, 5+ trials, >= 10 s.
4. Steady-state footprint (`footprint -p`) for candidate. 5. Then milestone runs on validation and all. 6. Ledger, report, activity log.
Acceptance: speedup vs baseline median with (max-min)/median reported; < ~5% = inconclusive; memory judged by steady-state footprint (baseline 2,172 MB), not RSS.

## Results

Setup: preserved baseline `parser_profile` vs **control** (unchanged `crf.c` recompiled and swapped into a copy of the baseline `libpostal.a`) vs **candidate**
(patched `crf.c`), all linked with the same harness source and flags. Each split: 1 burn-in round (recorded, excluded) + 7 measured rounds, variant order
rotated each round, 10 s per trial, 5 s gaps, single parsing thread, `/usr/bin/time -l`. All 72 runs exited 0. Raw per-trial tables:
`raw/bench-{development,validation,all}-summary.md`, raw outputs in `raw/bench-*/`. Machine: same M4 Pro, swap still ~40 of 42 GB used (`raw/bench-*/swap-*.txt`).

### Throughput (medians of 7 rounds)
**development (tuning split; no iterative tuning was done, this is the first and only design)**

| Variant | n | median addr/s | min | max | (max-min)/median | CV | median µs/addr | speedup vs baseline (medians) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 7 | 28,455.14 | 26,663.32 | 29,272.67 | 9.2% | 2.84% | 35.14 | 1.0000x |
| control | 7 | 27,123.62 | 26,284.02 | 29,160.98 | 10.6% | 3.92% | 36.87 | 0.9532x |
| candidate | 7 | 32,456.86 | 31,214.80 | 33,531.60 | 7.1% | 2.19% | 30.81 | 1.1406x |

paired per-round ratios (same round, back-to-back):
  control/baseline: 0.990 1.023 0.926 0.951 0.953 1.015 1.024 | median 0.9896 min 0.9258 max 1.0239
  candidate/baseline: 1.139 1.176 1.131 1.173 1.145 1.171 1.162 | median 1.1622 min 1.1313 max 1.1759
  candidate/control: 1.151 1.150 1.222 1.233 1.201 1.154 1.135 | median 1.1536 min 1.1350 max 1.2332
  baseline: peak footprint min 2,499,545,200 max 2,613,070,056 | max RSS min 2,207,956,992 max 2,613,690,368
  control: peak footprint min 2,514,274,440 max 2,613,201,152 | max RSS min 2,203,238,400 max 2,613,805,056
  candidate: peak footprint min 2,292,664,312 max 2,612,922,576 | max RSS min 2,115,485,696 max 2,613,690,368

**validation (milestone)**

| Variant | n | median addr/s | min | max | (max-min)/median | CV | median µs/addr | speedup vs baseline (medians) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 7 | 28,247.13 | 25,698.58 | 29,603.80 | 13.8% | 4.02% | 35.40 | 1.0000x |
| control | 7 | 27,662.23 | 26,630.58 | 28,559.71 | 7.0% | 2.04% | 36.15 | 0.9793x |
| candidate | 7 | 31,477.50 | 30,683.44 | 34,064.59 | 10.7% | 3.94% | 31.77 | 1.1144x |

paired per-round ratios (same round, back-to-back):
  control/baseline: 0.995 0.984 0.999 1.036 0.975 0.959 0.965 | median 0.9837 min 0.9589 max 1.0363
  candidate/baseline: 1.206 1.112 1.140 1.194 1.110 1.084 1.137 | median 1.1373 min 1.0835 max 1.2059
  candidate/control: 1.211 1.130 1.141 1.152 1.138 1.130 1.179 | median 1.1409 min 1.1300 max 1.2114
  baseline: peak footprint min 2,307,524,624 max 2,612,316,368 | max RSS min 2,190,721,024 max 2,612,707,328
  control: peak footprint min 2,301,446,136 max 2,612,496,616 | max RSS min 2,220,343,296 max 2,612,838,400
  candidate: peak footprint min 2,293,843,936 max 2,612,201,680 | max RSS min 2,293,284,864 max 2,612,559,872

**all (12,863, milestone; comparable to 000's corpus)**

| Variant | n | median addr/s | min | max | (max-min)/median | CV | median µs/addr | speedup vs baseline (medians) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 7 | 29,361.81 | 27,153.60 | 29,573.51 | 8.2% | 2.79% | 34.06 | 1.0000x |
| control | 7 | 29,324.18 | 28,558.29 | 29,473.90 | 3.1% | 0.99% | 34.10 | 0.9987x |
| candidate | 7 | 33,410.42 | 31,734.16 | 33,827.14 | 6.3% | 2.33% | 29.93 | 1.1379x |

paired per-round ratios (same round, back-to-back):
  control/baseline: 1.032 0.993 0.994 1.000 1.001 0.995 1.052 | median 1.0001 min 0.9926 max 1.0517
  candidate/baseline: 1.191 1.136 1.137 1.113 1.145 1.104 1.169 | median 1.1365 min 1.1041 max 1.1909
  candidate/control: 1.154 1.145 1.143 1.113 1.144 1.109 1.111 | median 1.1431 min 1.1092 max 1.1536
  baseline: peak footprint min 2,304,804,856 max 2,613,053,672 | max RSS min 2,264,760,320 max 2,613,837,824
  control: peak footprint min 2,277,296,168 max 2,613,086,416 | max RSS min 2,272,641,024 max 2,613,886,976
  candidate: peak footprint min 2,397,407,296 max 2,613,151,976 | max RSS min 2,285,715,456 max 2,613,919,744

Summary: candidate/baseline = **1.141x** (development), **1.114x** (validation), **1.138x** (all) by medians; paired same-round ratios 1.13-1.18 / 1.08-1.21 / 1.10-1.19,
so the candidate won every one of the 21 paired rounds and never fell below 1.08x. Time per address on `all`: 34.06 -> 29.93 us (-12.1%).
Noise: baseline (max-min)/median 8.2-13.8% between trials; the control ran 0.979-0.999x of baseline by medians (0.926-1.05 paired), i.e. rebuilding does not itself
speed anything up and the control-to-baseline gap shows the noise level. The effect is above the 5% inconclusive threshold set in 002, and is consistent
with 001's harness estimate (12.7%), though still ~2-3x the noise band on the noisiest split, so treat the exact figure as 1.11-1.14x.

### Memory
Steady-state footprint of the running candidate: **2,172 MB flat for 20 s after READY** (`raw/verify/footprint.txt`), identical to the baseline's 2,172 MB (002).
Peak footprints (`time -l`) overlap: baseline max 2,613,053,672 vs candidate max 2,613,151,976 bytes (`all`), ranges 2.29-2.61 GB for both. Peak RSS is not
used (swap noise, see 002). The change adds one reusable feature-id buffer (a few hundred to a few thousand uint32); not measurable. **No memory increase observed.**

### Correctness (hard constraint: byte-identical)
`tools/dump_outputs.c` (ordered label/component JSON per input, all 12,863 golden-v1 inputs in `all.txt` order) run against the preserved baseline library, the control
and the candidate: the three `raw/outputs_*.jsonl` files have the **same SHA-256** `45574ee2…c8c63` (byte-identical, `raw/outputs.sha256`). Candidate compared to the frozen
`golden-v1/baseline-outputs.jsonl`: 12,863 checked, 0 mismatches (`raw/golden-compare.txt`); negative control (one component deliberately altered) detected 1 mismatch.
Accumulation order is unchanged, so floating-point results are bit-identical by construction as well as by test.

### Single thread (hard constraint)
`ps -M` on the running candidate showed exactly 1 thread at all 6 sample points (`raw/verify/threads.txt`); CPU seconds ~ wall seconds (25.11 vs 25.16 s);
no thread/parallel primitives in the patch or source (only a comment mentions threads); the binary links only libSystem and has no `pthread_create`/`dispatch_apply` symbols.
The static feature-id buffer is not thread-safe, consistent with libpostal's already global parser state; this would matter only if parsing were ever made concurrent.

## Correctness/interpretation notes and limitations
- Mechanism (overlapped cache-missing trie walks) is not directly verified: no L1/L2/TLB counters (see 002 item 8). Evidence for it: locality experiment (001), the harness batching control, and the win being independent of any output change.
- Development/validation/all were all timed; the design was fixed before timing, so there is no tuning on development, but all three splits were observed (`all` was already used for baseline).
- Trials drift 3-9% within a session; control variance rounds 3-5 on development (0.93-0.95 of baseline) illustrate this. All trials are retained and none excluded except the labeled burn-in round.
- Single machine, single session, swap-pressured memory.
- Tests: no libpostal unit tests were run (not built in this tree); correctness rests on the exact-output checks above on the frozen corpus, which does not prove equality on every possible address (the change is a pure reordering of lookups vs accumulation, so equality holds by construction for any input).
- Failed/abandoned: none in this experiment. (Build note: the first `gcc -include /dev/null` flag is a harmless leftover in `tools/build.sh`.)

## Reproduction
`tools/build.sh` (control + candidate), `tools/dump_outputs.c`, `tools/run_bench.sh <split> 7`, `tools/analyze_bench.py <split>`, `tools/run_verify.sh`. Hashes: `raw/hashes.txt`, `artifacts/*/hashes.txt`.
Patch: `candidate/crf.patch` (original `crf.c.orig`, candidate `crf.c`); executables in `artifacts/{control,candidate}/`.

## Decision
**Accepted** as an incremental improvement: ~1.11-1.14x throughput (-12% time), byte-identical outputs, single thread, memory unchanged. The original 3.5x target is **not** reached
(would need ~9.7 us/address at ~29.93 us now; a further ~68% time reduction). Next: 004 = software-pipelined (interleaved) trie walks across several features per token (true memory-level parallelism, still one thread), then feature-string building
(`char_array_add_vjoined`, ~5% of total) and the phrase-trie search (`trie_search_tokens_from_index`, ~5-6%).
Ledger, report and activity log updated: yes.

**Note added after 004 (2026-09-29):** `crf_feature_ids_reserve` in this candidate returns NULL for an empty *first* batch (capacity 0 and n = 0), which would make `crf_tagger_score` return false. It never occurred in any run
(the first token always has features; all 12,863 outputs matched), and 004 fixes it, but the accepted 003 binary contains it. The measured 003 results are unchanged.
