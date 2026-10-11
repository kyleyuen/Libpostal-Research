# Architecture screen — ten diagnostic experiments (163–172)

Status: research screen, **no new parser or model built, no new speed or memory result**. The objective is an exact, single-thread address parser within the preserved original's peak and steady memory, with a design that removes work rather than moving it between stages. All new tables must derive from the model, not benchmark inputs.

## Evidence used

- The suite integrity stage in [172](../experiments/172-architecture-evaluation/README.md) passed for 11 non-reserved datasets, **110,551 addresses**, two enabled variants and seven model files. Registered v12/v13 remain reserved and were not opened. The original-parser O1 fuzz oracle has **178,955 parsed addresses** from its 200,000-input run; O2/O3i omit some shortcut/invalid cases by design. The stage oracles were made by the preserved original in [142](../experiments/142-stage-oracles/README.md).
- This screen ran read-only probes over those oracles, source and previously recorded model artifacts. Each numbered record keeps commands, raw output, exit status, input hashes and limitations. Earlier performance evidence is cited only from its original experiment; no diagnostic work count is treated as a speed result.

| Question | New observation | What it decides |
| --- | --- | --- |
| [163](../experiments/163-fused-normalize-tokenize/README.md): fuse normalization and tokenization | 24,733 / 20,741 / 471,781 token spellings changed in golden-v1 / OSM / fuzz O2 | Exact semantics matter; no evidence yet that fusion saves work |
| [164](../experiments/164-phrase-recognition-automata/README.md): replace phrase tries | 159's exact-search record covers all O1 addresses; 36 fuzz O2 omissions are whole-string shortcuts | A compact automaton remains untested; the exact hash design in 159 was slower and larger |
| [165](../experiments/165-direct-feature-emission/README.md): emit direct IDs | 702,370 golden and 546,890 OSM ordered O3i events; repeated IDs and misses are common | Repeated *scoring events* cannot be deleted merely because IDs repeat |
| [166](../experiments/166-offline-crf-reencode-design/README.md): rebuild the representation | Prior 160 compiler read back 13,557,097 feature rows bit-identically, zero unmapped | Exact offline compilation is feasible; 160's runtime was slower |
| [167](../experiments/167-sparse-layout-design/README.md): compress/rearrange weights | 65.5% of state values and 93.1% of transition values are distinct in 135's model probe | Value tables and dense rows lack a supported path; a different layout needs fewer accesses, not just fewer bytes |
| [168](../experiments/168-retraining-feasibility-design/README.md): train a smaller/new model | Upstream trainer and current model counts inspected; no reproducible labeled parser-training manifest identified in the retained tree | Retraining creates a new predictor and cannot guarantee the required identical output |
| [169](../experiments/169-decoding-architecture/README.md): replace decoding again | Real O1 token means 5.4–5.9; fuzz maximum 130; 20-label cell counts recorded | Adaptive pruning remains untested; fixed common-case scratch must handle long inputs |
| [170](../experiments/170-parser-architecture/README.md): integrate front half | O3i has 14,030,294 fuzz events, up to 1,146 in one address | A bounded ordered event stream is testable, with exact overflow behavior |
| [171](../experiments/171-model-memory-architecture/README.md): fit a new representation | 160 record arena is 1,117,339,520 bytes, plus 54,228,388 slot-ID bytes | File size is not resident memory; replacement and mapped-page accounting are mandatory |
| [172](../experiments/172-architecture-evaluation/README.md): choose by whole-system evidence | Suite inventory/integrity passed; no candidate run | Hard gates must include outputs, one thread, memory, cold/warm, setup, API/lifecycle and generalization |

## Most useful next architecture to test

**Hypothesis, not a result:** a standalone parser specialized to the unchanged model, carrying token and phrase identity into an ordered feature-event stream and scoring it directly. Keep the original phrase tries at first, because [159](../experiments/159-p1-exact-phrase-hash/README.md)'s exact hash search was slower and larger. Preserve every feature event, its order, score-row values, label tie rule, public API behavior and response formatting. The design should obtain shared IDs as part of context construction; [149](../experiments/149-rw-r1-bigram/README.md) showed that adding a separate canonicalization/ID lookup to cpi loses the expected saving, and [160](../experiments/160-p2-records-in-cpi/README.md) showed that an isolated record substitution also loses.

The first discriminating implementation should trace original events at the feature boundary, then consume them in a separate exact scoring path on a scratch copy. Compare O1/O2/O3i/O4 at the first differing address, including the 200k fuzz oracle, then compare ordered final outputs on the 11 non-reserved datasets and run API/lifecycle checks. Any difference rejects or explains the design before speed work. Count allocations, string bytes, probes and scratch growth to show that the proposed architecture actually removes work. A full candidate would then face the original-library output, single-thread, memory, setup, cold/warm and unseen-set protocol in [METHOD.md](../METHOD.md). Reserve v12/v13 for a selected milestone.

This screen does **not** establish a global optimum or a faster replacement. It narrows the next test to an integrated design whose central claim—shared identity without extra lookup work—can be falsified against the existing oracles and datasets. The professor report remains unchanged because no verified result changed.

## 2026-10-05 addendum: shared-identity implementation probe

[Experiment 173](../experiments/173-shared-token-identity/README.md) implemented the narrow vocabulary-slot sharing premise in scratch cpi sources. Across golden-v1, novel, OSM and fuzz, O2 eligibility was 58–66% of tokens; at runtime the exact lookup check found zero slot disagreements, and final outputs matched the original on those corpora plus a full 2M fuzz run. The full fuzz run skipped 7,908,725 repeated vocabulary lookups, with one parsing thread observed. This supports the availability of shared identity, not a speed result or a completed event-stream architecture.

The candidate's diagnostic paired full-fuzz peak footprint exceeded the preserved original by 79,544,512 bytes (2,518,337,816 vs 2,438,793,304); steady memory was not measured. Therefore this candidate is rejected under the memory constraint. Its extra footprint has not been isolated from inherited cpi memory, and neither ordered O3i/O4 equivalence nor full suite throughput was established. A memory-neutral integrated rebuild remains an open hypothesis.
