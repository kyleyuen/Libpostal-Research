# Experiment 001 — Component runtime audit (measurement only)

Date/time/timezone: 2026-09-29, 11:49–~12:40 PDT (raw start/end stamps in `raw/*.start`, `*.end`)
Status: **complete — diagnostic, no optimization, no speedup claimed**

## Hypothesis and change

Hypothesis carried from 000 (sampling profile only): CRF feature trie lookups dominate and are limited by
memory latency, not by instruction count. This experiment tests that and measures every pipeline stage.
Exact implementation change: **none to libpostal**. Only new measurement tooling: `tools/stage_bench.c`
(links the preserved baseline `libpostal.a`). Base = original libpostal commit `25099c50…`, empty source patch.

## Reproduction

Hardware/OS/compiler: Apple M4 Pro, macOS 26.5.2, Apple clang 17. Single thread, no other benchmark running.
Corpus: `profiling/golden-v1/development.txt` (10,308 addresses, development split only; nothing here is validation).
Model: default v1.0.0 files in `~/.cache/libpostal-profile/libpostal` (hashes in `profiling/manifest.json`).
Build: `raw/build-stage-bench.cmd` (`-std=gnu23 -g -O2`, same as library). Hashes: `raw/hashes.txt`.
Run: `./tools/stage_bench development.txt 10` (1 warm-up pass, then 10 measured passes; three runs, raw in `raw/stage-bench-run{1,2,3}.stdout`).
Counters: `xctrace record --template 'CPU Counters' … parser_profile development.txt 30` (`raw/xctrace-*.log`, exported tables `raw/counters20.xml`).

## Results

### 1. Per-stage cost (median of 28 passes, 3 runs; µs per address; sum 31.5)

| Stage | µs/addr | Share |
|---|---:|---:|
| Trie lookup: state features | 11.24 | 35.7% |
| Feature generation | 5.58 | 17.7% |
| `context_fill` (phrase/dictionary search) | 3.90 | 12.4% |
| Trie lookup: prev-tag features | 2.71 | 8.6% |
| Normalize (utf8proc/transliterate) | 2.56 | 8.1% |
| CRF context reset + transition copy | 1.32 | 4.2% |
| Weight accumulate: state | 1.29 | 4.1% |
| Weight accumulate: prev-tag | 0.83 | 2.6% |
| Viterbi | 0.70 | 2.2% |
| Tokenize + build tokens | 0.55 | 1.7% |
| Response build | 0.51 | 1.6% |
| Label strings, free temporaries | 0.33 | 1.1% |

Both trie lookups together are 44.3% of the staged time; trie + feature generation + context_fill are 74.4%.
These are exclusive, non-overlapping stage times (unlike the inclusive sampling shares in 000).

### 2. Workload counts (last 8 passes of run 1)
Per address: 5.88 tokens, 45.86 state-feature lookups (84.1% hit), 8.74 prev-tag lookups (95.4% hit).
A state-feature lookup walks a mean 21.4 trie nodes for 22.6-byte keys. The state trie has 22.1 M nodes,
68.8 MB of tail bytes and 11.5 M data entries; the prev-tag trie 3.8 M nodes.

### 3. Is the lookup memory-bound? (locality experiment on 472,695 recorded corpus features; 3 repetitions)

| Access pattern | ns/lookup |
|---|---:|
| Hot set (first 50 addresses' features, 2,333 keys, repeated) | 55–57 |
| Each lookup repeated back-to-back (mean of 1st and 2nd) | 139–161 |
| Corpus order | 182–192 |
| Shuffled order (no token-to-token locality) | 285–303 |

In-situ cost in the staged run is 11.24 µs / 45.86 ≈ 245 ns per lookup. The same code over a cache-resident key set
costs about 56 ns for the same ~21 dependent node reads, so roughly 70–80% of lookup time is waiting on memory
rather than executing. Shuffling makes it ~55% slower, so keys used by neighbouring tokens/addresses share cache lines
that the pipeline currently benefits from.

### 4. CPU Counters (Instruments, bottleneck mode), steady state t=6–14 s (`raw/counters-summary.txt`)
Columns as exported: 14.9% / 60.7% / 13.6% / 10.8%. Startup (model load) looks completely different (50–63% in column 1),
so the steady-state window was chosen from the per-second table. **Column meaning is inferred** from the exported
metric legend and area-graph spec as Useful / Instruction-processing bottleneck / Instruction-delivery bottleneck /
Discarded. Under that reading only ~15% of slots do useful work and ~61% are back-end (dependency/memory) stalled,
which agrees with section 3. No L1D/L2/TLB miss counts were captured, so cache vs TLB misses are NOT separated.

### 5. Control: does restructuring alone change speed? (unexpected finding)
Library call 35.47 µs/addr (median; range 33.4–40.5). Staged replica, timers off, original order
(lookup then accumulate per feature): 35.20. Staged replica, timers off, feature IDs looked up for a whole token first, then
accumulated: 30.72. Timers themselves cost ~1 µs (30.72 vs 31.69 with timers on).
So batching the lookups (same trie, same keys, same arithmetic) was **~12.7% faster in this harness**.
The likely mechanism is more overlapping cache-missing loads — UNVERIFIED. This is a harness result on development data,
not an implemented or output-checked library change.

## Correctness and interpretation
Staged replica vs `libpostal_parse_address`: 10,308 checked, 0 mismatches (3 inputs take the single-phrase bypass and
are delegated to the library). Trie step-count replica matched `trie_get` on every one of the 472,695 keys.
Development-only diagnostic; no validation data were used. Memory not measured (harness measurement only).

## Limitations
- Run 1 used an earlier revision of `stage_bench.c` (before the two control passes were added); that revision was not saved
  separately, only its output. Runs 2–3 match the saved source hash. Run 1's stage numbers are still included in the medians.
- Timings drift ±10% across passes (thermal/frequency); we report medians and ranges, 41.67 ns timer ticks are averaged.
- The stage breakdown describes the replica (batched lookups), which is 13% faster than the library, so shares are not
  exactly the original's. The library-time / staged-time relationship is in section 5.
- Locality experiment's hot set is a different key set than the corpus, so "56 ns" is an estimate of cache-resident cost.
- Counters are aggregated bottleneck categories, not per-function; the sampling profile in 000 remains the per-function view.
- `raw/*.trace` (640 MB) kept locally; exported tables are in `raw/`.

## Decision
Accepted as evidence (diagnostic). Nothing to accept/reject for speed. Recommendation: experiment 002 = batch/prefetch
feature lookups in `crf_tagger_score` with exact-output check on all 12,863 and RSS comparison.
Ledger, report and activity log updated: yes.

---
**Superseded note (2026-09-29, after 001):** the recommendation above to make batching experiment 002 is superseded.
The baseline was found incomplete for comparing candidates (no development-split throughput, no noise band, no steady-state memory,
stage table from the batched replica, inferred counter columns), so [002](../002-full-baseline/README.md) completes the baseline
and batching moves to 003. Sections above are unchanged.

**Evidence note (2026-09-29):** the two raw Instruments traces (`cpu-counters-probe.trace`, `cpu-counters-20s.trace`, 639 MB) were deleted
at the user's request because the disk filled up. The exported tables used for all counter numbers remain in `raw/`
(`CounterMetricAggregatedForProcess.xml`, `counters20.xml`, `counters-summary.txt`). The statement above that the traces are kept locally no longer holds.

**Correction (2026-09-29, from [002](../002-full-baseline/README.md)):** (1) The stage table in section 1 is the batched replica, which is ~13% faster than the
library; the original-order table is in 002 (lookup+accumulate = 56.6% of time, not 44%). (2) Counter columns 0 and 1 are confirmed
(Useful; back-end stall) by a calibration binary, but col1 is back-end stall of any cause (an ALU-only loop also scored 48%), so the counters
alone do not show memory-boundness; the locality experiment does. Cols 2/3 (delivery/discarded) remain unconfirmed. (3) The label
"free temporaries+response" excluded response destruction. Sections above are unchanged.
