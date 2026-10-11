# Experiment 187: CPI reuse-flow findings

## Result

The final4 diagnostic captured 8,903,320 logical query events over 128,540 addresses in 13 observed workloads. It recorded 4,039,344 physical dictionary searches, 546,206 existing CPI answer-reuse events, 4,317,770 precomputed feature-ID events, and 6,767,700 linked consumer events. The source-bundle proof establishes read-only dictionary/model use during one synchronous parse; it does not establish reuse across addresses, setup generations, concurrency, or reentrant calls.

Across the full collection, the pinned original oracle, release control, and diagnostic output were byte-identical for every workload. Golden-v1 also matched its frozen component reference on all 12,863 rows, and the deliberate negative control was detected. All sampled processes remained single-threaded.

## Repeated work and reuse

Exact repeated lookup identities account for 404,890 later physical requests (10.02% of the 4,039,344 physical calls). This is an upper bound on repeated searches; it does not show that those calls can safely be removed. The analyzer found no group with complete consumer coverage. The recorded repeat evidence is partial: 249,635 later requests have incomplete consumer links, 98,957 lack complete negative-result or consumer proof, and 56,298 have matching captured values under partial coverage.

The largest repeat strata are word-vocabulary canonicalization (304,202 later requests), state-feature lookup (93,395), vocabulary lookup (5,864), transition-feature lookup (778), and address-prefix/suffix phrase checks (651 combined). The canonicalization and feature-hash events carry a generic `instrumented_leaf` label, so they cannot yet be attributed to a specific producer callsite. Phrase wrapper booleans do not establish the contents or side effects of returned phrase data.

The 546,206 exact word-vocabulary memo flows show an existing CPI reuse path. The 4,317,770 precomputed IDs are direct role/constant values already supplied to consumers. Neither count is an incremental optimization opportunity. Further reuse work should begin with a small source-reviewed direct carry through existing per-token storage, link each carried scalar/ID to its actual consumer, preserve score multiplicity, and then remeasure a candidate against the pinned oracle.

## Storage and cost limits

An all-physical-search cache would require one probe opportunity per physical call (4,039,344). Repeated-key groups alone account for 360,009 first-seen keys with 4,479,619 key bytes in aggregate; known scalar/ID result payload widths sum to a 1,172,656-byte lower bound. These estimates exclude hash tables, pointers, allocation overhead, invalidation, copies, and code costs. They do not estimate runtime or RSS.

Across all addresses, p95/max live repeated-answer lower bounds are 2/26 entries, 66/905 key bytes, and 8/104 numeric payload bytes. The observed event span from first physical request to last linked consumer is p95 84 and max 486 rows. First-consumer-to-last-consumer lag was not separately measured. All byte figures exclude allocator and container overhead.

## Verification and record

The build/source identity and the detailed counters are in [`METRICS.md`](METRICS.md). Final4 source manifest SHA-256 is `016337d972512ba098591796044fa42323d3afcf534c9457b1e030bb63f6d33e`. The successful final4 golden and remaining12 raw command records are preserved under `raw/verification/`; remaining12 completed in 569.49 seconds with exit code 0. Reanalysis used saved traces only and is logged at `raw/verification/reanalysis-final4-v2-command.json`. The 13 reviewed analyzer fixtures passed; the exact test output and exit code are in `raw/verification/reviewed-analyzer-fixtures-final4.json`.

The earlier interrupted final3 and analyzer-only attempts remain preserved as unsuccessful history. No optimized candidate was built or timed. Therefore this experiment establishes workload-level reuse structure and bounded storage lower bounds, not a speedup, memory reduction, or optimal architecture.
