# Experiment 169 — exact decoding architecture options

Date/time/timezone: 2026-10-05T22:34:39-0700 (workspace clock; environment metadata labels the work date 2026-10-06).
Status: **diagnostic design study; no candidate built or timed**.
Hardware/OS: Apple M4 Pro, macOS 26.5.2, Darwin 25.5.0 arm64. No compiler invoked.

## Hypothesis and scope

The remaining decoder is already small (L=20, about 1,953 Viterbi cells per golden-v1 address), and the current max-only implementation (091) removed forward argmax bookkeeping. The useful open question is whether an **exact adaptive decoder** can reduce work for the common well-separated path while retaining original tie behavior, rather than another loop-order tweak. A proof-backed early exit may help, but decision margins and fallback cost determine whether it can.

Study alternatives: (A) current max-only + recompute argmax during backtrack (measured baseline); (B) interval-certified candidate pruning per DP column, with full 20-label scan when the certificate is inconclusive; (C) block checkpoint/backtrack recomputation to trade memory for work (expected poor fit at T≈6); (D) exact scalar/SIMD max-plus kernels. No alternative is selected as a winner.

## Evidence and rejected approaches

- 091 measured max-only/FMAX at about +2.0% over `pcp` on golden-v1 and +2.9% on OSM, while 091's own forward compare/select variant improved about 0.7% / 1.3%. Those gains establish that the decoder is tunable, not that new pruning wins.
- 012's source-label-outer Viterbi rewrite was slower by about 2.7% on golden-v1 and 3.3% on novel. 097's four-at-a-time accumulation was neutral/slower. Avoid restating these as unexplored options.
- 151 certifies reordered/f32 arithmetic for 100% of real paths and requires exact fallback on ties; 154 measured exact f64 K3 warm near 0.83 µs on golden-v1. The f32 path was rejected on memory, not arithmetic correctness.
- 162 killed broader parser redesign candidates. Decoder work must be tested in the full candidate context before claiming parser gain.

## Bounded oracle probe

`probe.py` reads O1 token-count records from golden-v1, novel, OSM v3 and the existing 200k fuzz oracle. It computes a dense L=20 cell-work proxy `max(T-1,0)*400`. Whole-string phrase shortcuts can skip prediction, so this is an upper bound, not observed decoder calls.

| Oracle | Parsed addresses | Mean T | Max T | Cell proxy total | Mean/address | Max/address |
|---|---:|---:|---:|---:|---:|---:|
| golden-v1 | 12,863 | 5.8819 | 28 | 25,118,400 | 1,952.76 | 10,800 |
| novel | 12,863 | 5.7345 | 33 | 24,360,000 | 1,893.80 | 12,800 |
| OSM v3 | 11,405 | 5.3623 | 27 | 19,900,800 | 1,744.92 | 10,400 |
| fuzz s1 | 178,955 | 7.6205 | 130 | 473,926,000 | 2,648.30 | 51,600 |

The concrete constraint is that fixed scratch sized for typical T≈6 fails the observed fuzz maximum T=130. The fuzz set contributes 473.9M potential cells and is a valuable long-sequence check. These are work counts, not timing results.

Exact command: `rtk python3 research/experiments/169-decoding-architecture/probe.py` followed by the four O1 gzip paths shown in [raw/commands.txt](raw/commands.txt). Exit 0; stderr empty; stdout [raw/probe.stdout](raw/probe.stdout). No build, model load, parser call or timing. Starting HEAD `181737d30977adc9d287e3f7a1cfafd7277525d4`; input manifest and source hashes are in [raw/source-hashes.txt](raw/source-hashes.txt).

## Constraints and next probe

Hard constraints remain byte-identical labels/components on every golden-v1 input and one parse thread. Next falsifiable probe: replay 142 O4 rows with a certified per-column prune plus full-scan fallback; compare every backpointer/label and count pruned cells/fallbacks on real + fuzz oracles. Kill on any mismatch or inability to prove the first-index tie rule. Speed remains untested and needs a later full candidate benchmark.

## Decision

No architecture decision. Proceed only with the bounded oracle replay described above; do not infer a speedup from 091, 151, or a theoretical cell reduction. Memory impact is expected small but unmeasured; output exactness and single-thread status are requirements, not assumptions.
