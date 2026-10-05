# Single-threaded libpostal optimization

## Research objective

Investigate a **3.5× throughput improvement** in address parsing while preserving
parser outputs and avoiding increased memory use. The proposed target is a
research objective; no optimized implementation has been measured yet.

## Current evidence

| Metric | Original baseline | Optimized result |
|---|---:|---|
| Median throughput | 27,821.28 addresses/s | Not measured |
| Median trial average latency | 35.94 µs/address | Not measured |
| Maximum peak RSS across trials | 2,613,886,976 bytes (2.434 GiB) | Not measured |
| Relative throughput | 1.00× | Not measured |
| Output preservation | Reference reproduced on 12,863 inputs | Not measured |

At the historical baseline rate, 3.5× corresponds to approximately **97,374
addresses/s**. Final improvement must use a contemporaneous baseline run alongside
the candidate, not only this historical number.

## Experimental setup

Apple M4 Pro, 24 GiB RAM, macOS 26.5.2, native arm64; Apple clang 17.0.0.
Libpostal commit `25099c506612b34b23b1bfe286ca6321fcf06f35`, default v1.0.0
model files. Original upstream build flags: library `-O2`, scanner `-O0`.

The frozen corpus contains **12,863 unique addresses from 88 countries**, including
5,399 non-ASCII inputs, derived from Senzing's public v1.2.0 test set. It is split
into 10,308 development and 2,555 validation inputs. Source records, supplied
annotations, checksums and exact original parser outputs are preserved.

Measured parsing plus response destruction on one thread. Model setup and one
full warm-up traversal were excluded. Three independent trials ran for at least
10 seconds each; all raw results were retained. Profiling was a separate run.

## Where time goes

The larger-corpus profile attributes **48.7%** of samples to trie lookups,
**15.3%** to feature generation and **2.3%** to Viterbi decoding. These are
inclusive shares and may overlap. Feature lookup is the first investigation
target; no cache-miss measurements or optimization gains are claimed yet.

### Component audit (experiment 001, diagnostic, development split)

Timing each stage separately (µs per address, sum 31.5): state-feature trie lookup 11.2, feature generation 5.6,
phrase/dictionary search 3.9, prev-tag lookup 2.7, normalization 2.6, everything else under 1.4 each; Viterbi 0.7 (2.2%).
A trie lookup costs about 245 ns in the pipeline but about 56 ns when its keys stay in cache, and CPU counters show
most steady-state slots stalled in the back end, consistent with memory-latency-bound lookups (column meaning inferred;
no cache-miss counts). This is measurement only: no optimization has been made or claimed.
Details: [experiment 001](../experiments/001-component-audit/README.md).
Correction from [experiment 002](../experiments/002-full-baseline/README.md): in the library's actual order, lookup plus weight accumulation is
56.6% of time (feature generation 15.8%, phrase search 10.7%, normalization 7.3%, Viterbi 2.0%). Baseline noise is 3-7.5% between trials,
steady-state memory footprint is 2,172 MB (peak RSS is unreliable when the machine swaps), and the counters show back-end stalls but do not
by themselves prove memory-boundness. Still measurement only.

## First optimization result (experiment 003)

Batching each token's feature lookups before accumulating weights (one function, `crf_tagger_score`) gave **1.11-1.14x throughput** (all corpus:
29,362 -> 33,410 addresses/s, -12% time per address) with **byte-identical output on all 12,863 inputs**, single-threaded, and unchanged
steady-state memory (2,172 MB footprint). It won all 21 paired trials against the baseline. This is an incremental result: the 3.5x target is not reached.
Details, raw trials and patch: [experiment 003](../experiments/003-batched-lookups/README.md).

## Current best result (experiments 003, 005, 006, 009, 010, 011; controlled re-baseline 007)

Byte-identical output (also on 4 million fuzzed inputs), one thread, no memory increase (peak footprint 2.557 GB vs baseline 2.613 GB; steady 2,438 MB); load time +3.7 s.

| Corpus | Baseline addr/s | Current best addr/s | Speedup |
|---|---:|---:|---:|
| golden-v1, all 12,863 | 27,981 | 73,801 | **2.66x** |
| novel set (12,863 unseen strings) | 27,322 | 73,746 | **2.71x** |
| validation split (2,555), frozen harness | 27,760 | 75,024 | **2.70x** |

Changes: (003) look up a token's feature ids before accumulating weights; (005) remove redundant work (per-byte appends, double zeroing, duplicate lookups); (006) at load, replace the two feature tries with an exact hashed dictionary of the model's own keys
(names stored once, full-key verification, safe fallback to the trie), looked up in batches. Measured in a controlled, interference-aware session on a shared laptop (baseline and all variants interleaved; details in experiment 007);
the earlier baseline reproduces within ~1%. Per-country speedup on the cold first pass stays between 1.6x and 1.9x. The original 3.5x target is not yet reached (about 20 us per address remain: feature generation, phrase search, normalization, vocab lookups).
Later changes: (009) keys generated straight into one buffer with weight-row prefetch, (010) exact vocabulary dictionary + faster feature-string join + lean Viterbi setup, (011) exact ASCII fast path for string normalization (covers ~58% of our addresses; the gain shrinks on a less-ASCII dataset, est. 2.36x at 0% ASCII to 2.84x at 100%).
Details: [006](../experiments/006-feature-dict/README.md), [007](../experiments/007-controlled-rebaseline/README.md), [009](../experiments/009-direct-keys/README.md), [010](../experiments/010-vocab-dict/README.md), [011](../experiments/011-normalize-fastpaths/README.md).
Provisional (2026-09-30, pending a 20-round paired confirmation): (015) integer-keyed lookup for word features measured **2.745x (golden-v1) / 2.741x (novel)**
vs the baseline in one controlled session, +2.6% / +2.1% over a rebuilt `h` control; byte-identical (datasets, 4 M fuzz inputs, 280 M lookups checked against
the unsplit dictionaries); one thread; peak footprint 2.538 GB. Not yet folded into the table above. Details: [015](../experiments/015-integer-keys/README.md).

## Limits of the evidence

This is a finite, country-skewed public test corpus repeatedly processed after
warm-up, not a production traffic sample. Training overlap is unknown. Matching
the original parser checks behavioral preservation, not annotation accuracy or
correctness on every possible address. Memory results currently measure peak RSS;
future acceptance also needs consistent memory measurements and noise checks.

## Evidence and next comparison

- [Baseline experiment, individual trials and preserved binaries](../experiments/000-baseline/README.md)
- [Dataset definition and provenance](../../profiling/golden-v1/README.md)
- [Detailed profile and raw measurements](../../profiling/golden-v1/RESULTS.md)
- [Experiment ledger](../experiments/README.md)
- [Measurement and acceptance method](../METHOD.md)

Each completed optimization will add a ledger entry with its change, rationale,
source patch, raw trials, speedup, memory difference, output check and decision.
The final report will identify which changes produced the improvement and which
experiments failed or were discarded.
