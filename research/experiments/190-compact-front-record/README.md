# Experiment 190 — keep only derived values in the front record

Date/time/timezone: 2026-10-07 America/Los_Angeles; hypothesis recorded before source edit.
Status: completed implementation attempt; release build and development/golden output/thread/frozen checks passed. Requested parser-allocation bytes fell by 3,456/4,160 (development/golden), equal to 32 bytes per front-record capacity element; lookup counts were unchanged. Timing and process-memory acceptance are unmeasured.
Protocol: `research/METHOD.md`.

## Hypothesis and change

181's per-token `ap_front_t` stores `word`, three `int64_t` phrase-membership indices, vocabulary frequency and state feature ID. The word pointer and all three phrase memberships already exist in `address_parser_context_t`; copying them into another record duplicates storage and reads. Keep only `{freq, word_id}` (8 bytes) and read the word and phrase indices directly from existing context arrays at feature generation. This is dataflow from the same parse context, not a result cache or input-derived shortcut.

Exact change: reduce front records from their current 40-byte layout to 8 bytes, remove redundant member writes, and use existing context arrays for the four duplicated values. Preserve the old fallback when front allocation fails.
Base: commit `181737d30977adc9d287e3f7a1cfafd7277525d`; source hashes in `raw/base_hashes.sha256`.

## Operation-based research diagnostics

Predicted changes per token: remove one stored pointer and three 64-bit indices (32 bytes) and four record reads/writes; add direct reads from the existing context arrays when those values are used. At front capacity growth, requested realloc bytes fall from `n*sizeof(old record)` to `n*8`. Frequency and direct-ID lookups stay unchanged.
Metric units/coverage: front-record bytes allocated; member copy/load counts; feature lookup calls; one per parse address and per token. Diagnostic counter source is delegated to benchmark_186 using 185 hooks, or marked unavailable when not instrumented.

| Metric | 181 control | Candidate | Evidence |
|---|---:|---:|---|
| Front record size | 40 B/token by source layout | 8 B/token by source layout | source audited |
| Lookup calls, dev/golden | 848,149 / 1,058,689 | unchanged | diagnostic counters |
| Instrumented requested allocation bytes, dev/golden | 934,944 / 1,166,110 | 931,488 / 1,161,950 | −3,456 / −4,160 B (parser_alloc; equals 32 B per front-record capacity element) |
| Allocation calls / model rows / weight additions | unchanged | unchanged | diagnostic counters |

## Reproduction

Apple clang and exact build flags are in `raw/build_flags.txt`; command captures are `raw/build_release.*`; source and binary manifests remain in `raw/`.
Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.
Parser invoked by implementation author: no.

## Correctness and interpretation

Development and golden output/thread checks passed. The allocation byte delta is entirely in the 185 `parser_alloc` category; `crf_alloc` remains 683,184 B (development) and 852,150 B (golden). The reduction equals 32 fewer requested bytes per front-record capacity element, not per processed token; it is not a peak or steady RSS measurement. Exact values and raw captures are in `research/analysis/parallel-exploration-2026-10-07/REPORT.md` and `raw/190-*`.

Context values remain scoped to the same address parse and are read at the original consumers. This source argument does not certify all public API/lifecycle behavior. Output identity, single-thread, speed and original-memory acceptance await the delegated benchmark.

## Decision

Pending release build and serialized screen.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. Diagnostic allocation requests fell by 3,456 bytes in development and 4,160 bytes in golden; lookup counts remained 848,149/1,058,689. The byte delta is 32 bytes per front-record capacity element from the 40-byte to 8-byte record, not 32 bytes per processed token. It is requested allocation volume, not process footprint. Development/golden output and sampled one-thread checks passed. No timing or memory gate was run. All build and screen attempts remain preserved under `raw/`.
