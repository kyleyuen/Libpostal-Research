# Experiment 188 — stream resolved events into score arrays

Date/time/timezone: 2026-10-07 America/Los_Angeles; pre-registered before source edit.
Status: completed implementation attempt; release build and development/golden six-role output/thread/frozen checks passed. Diagnostic counters show unchanged lookups, model rows and weights and one fewer CRF allocation request (1,024 fewer requested bytes) on golden. Event-array traffic, timing and process-memory acceptance are unavailable; no promotion claim.
Protocol: `research/METHOD.md`; candidate parser/model runs are delegated to benchmark_186.

## Hypothesis and change

Experiment 183 found the 181 candidate's maximum peak footprint was 1,720,320 bytes above original. In 181, `crf_tagger_score` allocates and fills `original_events` (one `{id,trans}` record per state and transition feature), then rereads it to update score arrays. The state event order, followed by transition event order, is already the order of the original scorer. Stream lookup and score accumulation in those same loops, injecting the carried direct word ID at its original feature index. This should remove the event staging allocation and its write/read pass while preserving per-score addition order.

Exact implementation: replace the 181 `original_event_t` staging path with a state-feature loop that resolves and scores one event at a time, followed by a transition-feature loop; apply each CSR row in its existing order. Retain misses, direct-ID insertion position, duplicate events and original fallback path if the direct ID does not apply.
Base: Git commit `181737d30977adc9d287e3f7a1cfafd7277525d4`; exact 181 source copy hashes are in `raw/base_hashes.sha256`.

## Operation-based research diagnostics

Predicted changes: eliminate one event-array record write and read per event, plus its capacity growth/realloc and retained high-water allocation. Trie lookups and score-weight additions should be unchanged. No global or cross-address cache.
Metric definitions: event records allocated/bytes/growth; event staging writes and reads; feature lookup calls; weight additions. Scope: full address parse, reset each run; candidate screen workload and then delegated golden/13 coverage.
Release build: `raw/build_release.*`, exit 0, 2026-10-07 18:55:13–18:55:22 PDT; Apple clang 17, `-O2`, `EXP_FUSED_FRONT`, `EXP_EVENT_FLOW`, system allocator. First compile failed on duplicate same-scope score pointer declarations; all logs retained under `raw/build-attempt1/`. The retry renamed only stream-local pointers and compiled successfully. A matching diagnostic derivative is being prepared by benchmark_186 from the 185-instrumented prototype181 tree with the same patch.
Diagnostic versus uninstrumented output checks and negative control: delegated to benchmark_186.

| Metric | Control | Candidate | Evidence |
|---|---:|---:|---|
| Feature event staging records | unavailable | unavailable | 185 hooks do not record event writes/reads or capacity |
| Instrumented lookup calls | 1,058,689 | 1,058,689 | unchanged |
| Model rows / weight additions | 603,376 / 14,045,912 | same | unchanged |
| CRF allocation calls / requested bytes | 86,751 / 1,166,110 | 86,750 / 1,165,086 | one fewer call, 1,024 fewer requested bytes; not RSS evidence |

Golden values, output parity, and limits are documented in `research/analysis/parallel-exploration-2026-10-07/REPORT.md` under Experiment 188. No timing or RSS improvement is measured; event-array traffic remains unavailable, and the single allocator-request delta does not establish lower process footprint.

## Reproduction

Hardware/OS/compiler: Apple Silicon macOS; Apple clang 17; release flags and complete command capture are in `raw/build_flags.txt` and `raw/build_release.*`.
Corpus/model/source identity: experiment 183 corpus/model manifest; candidate parser runs delegated, not yet performed.
Build command/log: `raw/build_release.*`; patch: `raw/crf.patch`; hashes: `raw/source_manifest.sha256` and `raw/final_hashes.sha256`. Candidate `.a`, `.dylib` and dumper artifacts are retained. The diagnostic build is delegated to benchmark_186.
Parser invoked by implementation author: no.

## Correctness and interpretation

Development and golden six-role output/thread parity passed under benchmark_186. Golden counts were unchanged from 181: 1,058,689 lookup calls, 142,241 repeats, 603,376 model rows, 14,045,912 weight additions, 29,608 direct IDs, and 3,509 fallbacks. The operation cut was not demonstrated by query/row/weight counters; event-record traffic remains uninstrumented and needs source-based accounting or an explicit unavailable result. Speed and memory verdict remain pending exact benchmark summary; no gain is inferred from the source change.

## Decision

Release build succeeded; first compile failure is retained. Benchmark-agent evidence pending. Do not infer speed or memory improvement from the removed array alone.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. The release and matching diagnostic screens passed development and golden output/thread gates, including frozen golden output. Golden operation metrics versus the 181 diagnostic control were unchanged at 1,058,689 lookups, 142,241 repeats, 603,376 rows, and 14,045,912 weight additions; instrumented CRF allocation requests fell by one and requested bytes by 1,024. The 185 hooks do not count event-record writes/reads or event capacity, so the hypothesized staging-work reduction is not independently measured. No speed or footprint result is available. The initial helper/output-directory failure and retry are retained in `raw/`; no failed attempt was replaced or deleted.
