# Experiment 180 — integrated carried-identity events

Date/time/timezone: 2026-10-05T23:48:40-0700 PDT, pre-registered before source edits.
Status: **semantic vertical slice verified; no speed result**.

## Hypothesis and work-removal metric

Using 177's original-source front record and 178's original event scorer, carry the model-resolved state `word|TOKEN` ID from context fill to feature generation. For an in-vocabulary word, feature generation injects that ID directly at the original event position, bypassing both its repeated `feature_array_add(..., "word", word)` construction and its state-feature trie lookup. Every other feature and every unavailable record uses the original string/trie path.

Metric: count direct state-word events and fallback state-word events. Semantic gates: byte-identical O1/O2/O3i/O4 and final outputs on golden, novel, OSM and bounded fuzz; direct stream must preserve the original feature position. No timing or speed claim.

## Diagnostics completeness

- `raw/thread_static_scan.txt` records a static scan of the changed source. It is not a live thread-count observation; no `ps -M` capture was collected, so the observed runtime thread count is unavailable.
- No memory command or output artifact (`/usr/bin/time -l` or `footprint`) was collected. Peak and steady memory are therefore unavailable/inconclusive.
- Semantic checks are complete for the corpus comparisons listed below. Live one-thread and memory diagnostics remain incomplete. This documentation follow-up started no new run.

## Decision

The prototype is a real integrated work-removal path: context fill computes and carries the state `word|token` identity, and the feature callback omits the original word-feature string while the event scorer injects the carried ID at that position. It is not a side-by-side buffer merge.

The repaired golden run passes O1, O2, O3i, O4 and final output byte comparison. Novel and OSM pass the same five checks. Bounded fuzz seed 1 (200,000 inputs) passes O1/O2/O3i/O4 and its output matches the preserved-original 200,000-line prefix. The O3i writer was extended to render the omitted direct event at its original index; this is the checked representation, not a relaxed oracle.

The golden process reports 29,608 direct state-word events and 3,509 fallback state-word events. The fuzz process reports 500,808 direct and 52,287 fallback events. Each direct event bypasses the original `feature_array_add(features, 2, "word", word)` call and the later `crf_get_feature_id` trie lookup; context fill performs one model trie lookup to establish the carried ID. No speed conclusion follows from counts.

The missing-directory invalid run remains in `raw/golden.*`; the verified rerun is `golden_o3fix.*`. Source patch, hashes, command logs, failures, and the static thread scan remain in this directory only.
