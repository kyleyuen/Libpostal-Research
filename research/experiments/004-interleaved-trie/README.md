# Experiment 004 — Interleaved (software-pipelined) trie walks for CRF feature lookup

Date: 2026-09-29 PDT (development timing ~13:00-13:14). Status: **inconclusive / rejected as an increment (no gain over 003 beyond noise); milestone runs not performed**

## Hypothesis and change
003 overlapped lookups only implicitly (all walks for a token issued before accumulation). Each walk is a chain of ~21 *dependent* node loads plus a data-entry load and a
tail load; the CPU can only overlap walks as far as its out-of-order window reaches. 004 makes the overlap explicit: `trie_get_data_batch` (new, `trie.c`) advances up to L lanes
one trie step per round, issuing `__builtin_prefetch` for each lane's next node / data entry / tail before any lane consumes it. Single thread; no output change.
Semantics must equal `trie_get_from_index` + `trie_get_data_at_index` exactly (same wrap-around index arithmetic, same bounds behaviour, same tail compare via `trie_compare_tail`).
`crf.c` (on top of 003's change) builds a pointer array of the token's feature strings and calls the batch function. Lane count L is a compile-time constant; L=4/8/16 are built and
compared as an ablation (all trials kept). Base: original libpostal; incremental base: 003 (`candidate/crf.c.003`); cumulative patch vs original saved too.
Hard constraints (METHOD.md): byte-identical outputs on all 12,863; single thread (verify `ps -M`, no thread primitives).

## Plan
build lanes 4/8/16 + control(=003) ; dump-output identity check for each ; development-split alternating timing vs baseline and 003 ; pick the best L on development, then
milestone (validation, all) ; steady footprint + thread check ; write-up, ledger, report.

## Results

Correctness (hard constraint): L4, L8 and L16 outputs are **byte-identical** to the baseline on all 12,863 inputs (same SHA-256 `45574ee2…c8c63` as 003's baseline/candidate outputs; `raw/outputs.sha256`).
Single thread: code review only (no thread/parallel primitives; prefetch and lane interleaving are within one thread). A runtime `ps -M` check was not run because the candidate was not carried to the milestone stage.

Timing (development split only; baseline, 003 candidate (`c003`) and L4/L8/L16 rotated per round, 1 burn-in + 7 measured rounds, 10 s trials, 40 runs all exit 0; `raw/bench-development-summary.md`):

| Variant | n | median addr/s | min | max | (max-min)/median | CV | median µs/addr | speedup vs baseline | vs c003 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 7 | 28,947.04 | 28,198.84 | 29,959.05 | 6.1% | 1.98% | 34.55 | 1.0000x | 0.8639x |
| c003 | 7 | 33,507.31 | 32,637.78 | 33,948.92 | 3.9% | 1.34% | 29.84 | 1.1575x | 1.0000x |
| L4 | 7 | 32,265.38 | 31,225.70 | 32,921.78 | 5.3% | 2.03% | 30.99 | 1.1146x | 0.9629x |
| L8 | 7 | 33,807.23 | 33,197.31 | 34,181.95 | 2.9% | 0.84% | 29.58 | 1.1679x | 1.0090x |
| L16 | 7 | 33,380.43 | 31,703.97 | 34,734.97 | 9.1% | 2.54% | 29.96 | 1.1532x | 0.9962x |

paired same-round ratios:
  c003/baseline: 1.173 1.105 1.174 1.178 1.174 1.118 1.163 | median 1.1728 min 1.1055 max 1.1776
  L4/baseline: 1.135 1.115 1.102 1.115 1.101 1.077 1.123 | median 1.1145 min 1.0770 max 1.1349
  L4/c003:     0.968 1.009 0.938 0.946 0.938 0.963 0.966 | median 0.9629 min 0.9382 max 1.0087
  L8/baseline: 1.147 1.158 1.169 1.199 1.189 1.135 1.159 | median 1.1594 min 1.1351 max 1.1989
  L8/c003:     0.978 1.047 0.995 1.018 1.013 1.015 0.997 | median 1.0130 min 0.9779 max 1.0473
  L16/baseline: 1.159 1.138 1.144 1.124 1.177 1.159 1.127 | median 1.1440 min 1.1243 max 1.1773
  L16/c003:     0.988 1.029 0.974 0.955 1.003 1.037 0.969 | median 0.9880 min 0.9547 max 1.0366
  baseline: peak footprint min 2,277,132,376 max 2,612,857,040 | max RSS min 2,250,686,464 max 2,613,641,216
  c003: peak footprint min 2,321,238,008 max 2,612,824,320 | max RSS min 2,203,435,008 max 2,613,592,064
  L4: peak footprint min 2,336,393,232 max 2,613,135,592 | max RSS min 2,154,987,520 max 2,613,837,824
  L8: peak footprint min 2,351,220,752 max 2,612,840,680 | max RSS min 2,350,645,248 max 2,613,657,600
  L16: peak footprint min 2,376,304,680 max 2,612,726,016 | max RSS min 2,151,596,032 max 2,613,510,144

Reading: every variant beats baseline by 1.11-1.17x (that is 003's batching effect), but **none beats 003 by more than noise**: L8 vs c003 = 1.013x (paired range 0.978-1.047), L16 = 0.988x, L4 = 0.963x
(consistently slower). Baseline noise here is 6.1% (max-min)/median, so anything under ~5% is inconclusive under the 002 rule.

## Interpretation and limitations
- Explicit prefetch interleaving of a token's ~8 feature keys did not help. Possible reasons (all UNVERIFIED, no L1/TLB counters): the out-of-order core already overlaps the independent walks that 003 issues back to back;
  lane bookkeeping (state machine, extra branches) cancels the extra overlap; per-token batches are too small (mean ~8 state keys) to fill more lanes; or the remaining cost is not simple DRAM latency.
- **This weakens the working explanation for 003's gain.** If more explicit memory overlap adds nothing, then 003's ~12-15% may come from restructuring the loop (e.g. separating trie loads from the dependent weight stores) rather than from memory-level parallelism.
  This is untested. It also lowers the expected value of cross-token batching (which would only add lanes).
- No milestone (validation/all), footprint or `ps -M` runs were done, by design: the acceptance rule was not met on the development split, and running them would not change the decision.
- The candidate carries the small robustness fix to the shared buffer helper (`|| crf_feature_ids == NULL`, so an empty first batch cannot fail); it is not the cause of any measured difference.

## Reproduction
`tools/build.sh` (L4/L8/L16), `tools/run_bench.sh development 7 baseline c003 L4 L8 L16`, `tools/analyze_bench.py`. Patches: `candidate/{crf.cumulative,crf.incremental,trie,trie.h}.patch`. Hashes: `raw/hashes.txt`, `artifacts/L*/hashes.txt`.

## Decision
**Inconclusive / rejected as an increment**: not adopted; 003 remains the best candidate (1.11-1.14x). Retained as a documented failed attempt.
Next: measure before building more. (1) Determine what part of 003's win is loop restructuring vs memory overlap (e.g. a variant with batching but sequential dependent walks vs hardware-independent alternatives), (2) measure constant-prefix length and cold-step counts per lookup,
(3) exact, low-risk wins outside the trie walk: vocab lookup caching per token (each word is looked up up to 3x), feature-string construction, normalization ASCII path.
Ledger, report and activity log updated: yes.
