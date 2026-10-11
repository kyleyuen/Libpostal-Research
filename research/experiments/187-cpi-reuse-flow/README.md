# Experiment 187 — CPI repeat queries and consumers

Date: 2026-10-07, America/Los_Angeles. Started17:07:36.528246 PDT.
Status: complete; research hypothesis partially supported, replacement unproven. Evidence: research diagnostic.
Protocol: research/METHOD.md; final output, speed, memory and single-thread gates unchanged.

## Hypothesis and change

Trace remaining CPI searches through actual consumers before choosing a different architecture. Determine which requests repeat, which values CPI already carries, and what storage/bookkeeping a replacement would require. No new parser cache or optimized architecture was implemented. Diagnostics-only isolated source is preserved under source/; original000, final119 and historical experiments remain unchanged.

Base Git commit:181737d30977adc9d287e3f7a1cfafd7277525d4. Source derivative:185 CPI plus expanded query/consumer hooks. Final4 source-manifest digest:016337d972512ba098591796044fa42323d3afcf534c9457b1e030bb63f6d33e. Complete source, hash manifests, commands and provenance are retained; do not infer packaged119 runtime characteristics from these diagnostic builds.

Full findings: [REPORT.md](REPORT.md), detailed measurements: [METRICS.md](METRICS.md), reviewed analysis: [overall-analysis.json](results/reviewed-final4-v2/overall-analysis.json).

## Measurements and definitions

Exact repeated queries group by dictionary, operation, family and key bytes, plus any reported dictionary/model generation, within one address. Physical searches, existing token-memo reuse and precomputed feature IDs are separate lanes. Repeated score contributions must still be applied. Consumer mappings are explicitly partial; phrase boolean results do not capture their complete output arrays/effects. A named source proof establishes read-only model/dictionary access during one synchronous parse; cross-address/re-setup/concurrent/reentrant reuse is unproven. Numeric generation values are unavailable.

| All13 measured family | Physical searches | Repeated physical searches, upper bound |
|---|---:|---:|
| Vocabulary |1,322,284|310,066|
| State feature |1,859,856|93,395|
| Transition feature |70,173|778|
| Address phrase |529,951|651|
| Component phrase |128,540|0|
| Postal phrase |128,540|0|
| Total |4,039,344|404,890|

Existing token-memo answers:546,206, with zero observed source-answer disagreements. Precomputed state/transition IDs:4,317,770. Trace contains8,903,320 query/request rows and6,767,700 consumer rows. Repeat evidence:56,298 matching requests at covered sites with partial overall coverage;249,635 with incomplete consumer links;98,957 with incomplete negative-answer/consumer proof. These are diagnostic classifications, not proven search removals or speed gains. At most10.02% of observed physical searches repeat; about76.58% of those repeats are vocabulary requests.

Operation coverage differs from185; counts across experiments must not be treated as a parser change or directly compared without scope reconciliation. Constructor/copy/traversal/allocation/hardware metrics absent from these hooks remain unknown, not zero. Proposed live payload/cache-operation costs are estimates, not measured parser memory or runtime.

## Reproduction and evidence

- [Build and exact flags](raw/build-187-instrumentation-final4/): Apple clang at/usr/bin/clang, library-O2/scanner-O0, exact119 delivery switches. Matching unchanged-source release control and diagnostic pair. Command:tools/build_trace.sh --build. Actual build17:59:37–17:59:57 PDT, exit0.
- [Runtime calibration](raw/calibration-event-contract-6/): exact per-command records, typed miss-zero, restored call sites, query/consumer links, framing and forced overflow rejection.
- [Golden collection](results/golden-preservation-build-187-final4/results.json): actual start18:01:35 PDT,49.876seconds. Original000, release-control and diagnostic outputs agree;12,863 frozen rows, zero mismatches; induced one-component mismatch detected.
- [Remaining12](results/remaining12-build-187-final4/results.json):18:02:53.516365–18:12:23.011564 PDT,569.49seconds, wrapper exit0. Sequential corpus collection; exact commands/times in raw/verification. Compression is lossless and decoded hashes were verified before removing only each newly generated uncompressed remaining12 trace. Golden raw trace retained.
- [Coordinator audit](raw/verification/coordinator-all13-audit.json):13 unique datasets,128,540 address executions, all analyses complete, all byte checks identical and every sampled thread count1.
- Each run manifest records source/config/binary/model/input hashes before and after, environment gates and trial commands. Streaming per-address metrics and raw traces are retained.
- [Source/reuse contract](REUSE_CONTRACT.md), [instrumentation limits](INSTRUMENTATION.md).

These are collection durations including diagnostic overhead, not final speed trials. Corpus-specific timings must not be pooled into a performance claim. No final peak/steady footprint acceptance was run.

## Failures retained

Initial missing-return/format builds, calibration/adapter failures, smoke recovery attempts and final3 captures remain in raw/results. Final3 had stale phrase site attribution and untyped zero-on-miss vocabulary values; final4 corrected only trace metadata and passed calibration/output checks. A final3 remaining12 launch was interrupted on receipt of the trace-fix hold; its incomplete command/result records remain. Earlier evidence was not rewritten as successful. Calibration4 asserted the wrong restored site; calibration5/6 passed corrected expectations.

## Decision

Hypothesis partially supported: substantial existing reuse and remaining vocabulary/state repeats observed; complete consumer equivalence and profitable replacement are unproven. No candidate promotion, new speedup or memory-cap compliance established. Leading follow-up: investigate carrying exact token attributes through existing per-address storage, preserving all feature occurrences, and compare its added work against generic cache probes/copies. Close missing consumer/effect coverage before claiming safe removals. Do not tune to country/source/key lists; all13 datasets are observed, not fresh holdouts.

Professor performance claims remain unchanged. Reviewed analyzer13/13 fixtures pass. Independent source/schema and all13 record/hash audits complete. An initial analyzer-only reanalysis with misaligned consumer provenance was retained as unsuccessful; final synthesis uses reviewed-final4-v2 with corrected tuple/type/unknown-generation/operation fields.

Storage estimates for typed repeated answers with linked consumers: p95/max live slots2/26, exact key bytes66/905, numeric payload bytes8/104; first physical request to last linked consumer lifetime84/486 event rows. Unlinked/untyped requests and container/allocator costs are excluded; first-consumer-to-last-consumer lag remains unavailable. No memory gate inferred.

Ledger, activity log and dated handoff updated. Professor report remains unchanged.
