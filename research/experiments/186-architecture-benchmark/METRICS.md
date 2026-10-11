# Experiment 186 trace and architecture metrics

## Collection contract

`tools/run_architecture_benchmark.py` is a stdlib CLI. The default invocation is
preflight only; `--run` requires one explicit `--mode smoke|development|golden-v1-acceptance|full|newedge`.
Each workload runs serially through the preserved 000 dynamic-library oracle,
an uninstrumented source-matched release control, and the diagnostic build. The
release and diagnostic output files must each match the oracle byte for byte.
When a frozen golden-v1 output reference is declared, its semantic comparison
is also checked with the 185 comparator, including a negative control. The 185
AC/thermal process gate and thread sampler run before every child;
no observed sample is incomplete evidence and any observed thread count other
than one fails the record. Trace-enabled diagnostic runs write bounded JSONL
traces using `TRACE_JSONL`, `TRACE_EVENT_CAP`, and declared expected address
counts. `--analyze TRACE` reads that trace without launching parser processes.

Result directories are created with exclusive `mkdir`; prior results are never
overwritten. The runner copies its source and config and retains raw commands,
stdout, stderr, exit status, start/end timestamps, `time -l` evidence, thread
samples, input/model/library/provenance hashes, output hashes, and trace hashes.
Process durations are provenance only. The runner does not build binaries.

## Trace schema v1

JSONL must contain exactly one `trace_header`, zero or more `query` and
`address_summary` rows, then exactly one `trace_end`. `trace_overflow` rejects
the trace. Blank/malformed rows, undeclared records, duplicate identities,
missing address summaries, address-count mismatch, query-summary disagreement,
or any false/missing completeness flag also reject it. Event and address caps
are declarations, not permission to truncate.

`trace_header` declares schema, variant, event cap, overflow policy, and key byte
encoding. `query` records distinguish logical requests from actual physical
searches and answer-reuse events. Search identity is dictionary identity plus
generation, operation family and semantics, and exact query bytes. Unknown dictionary
generation must carry a reason. A repeated query with unknown/mismatched
generation or without an explicit lifetime proof is labeled unproven; it is not
counted as a safe reuse. `address_summary` declares its SHA-256 input identity,
completeness flags, available counters, explicit null reasons, and output hash
when instrumented. Raw input is not stored; the address identity is a hash of
the effective input bytes. Query keys are stored as exact hexadecimal bytes,
which are reversible and may contain sensitive query text.

The implemented collector reports `safe_reuse_proof: unknown` for
all search events and explicit diag-reuse source sites without proving semantic
cache validity. Such events can support architecture guidance only; they do
not establish correctness of a cache or savings. Counters with no instrumented
boundary, including output hash when not emitted by the trace producer, are
null with a reason, not zero.

## Metrics and boundaries

| Metric | Definition | Interpretation |
|---|---|---|
| Logical requests | Emitted query events (`physical_search` + `answer_reuse`) | Requests observed by instrumented hooks only; incomplete call-site coverage remains partial. |
| Physical searches | Query events that actually invoke a trie/dictionary/model search | Sum across stages; stage movement does not remove work. |
| Executed repeat searches | Physical searches repeating exact dictionary/op/key identity within one address | Candidate duplication signal; not necessarily redundant work. |
| Generation-valid repeat searches | Repeat with known same generation and explicit valid reuse proof | Only this subset can inform safe reuse. |
| Answer reuse | Explicit no-search answer event | Distinct from first-use precomputed IDs; semantic validity must be separately established. |
| Key/text constructions and bytes | Instrumented materialization boundary counts/bytes | Incomplete constructor and intermediate-copy scopes stay unavailable. |
| Allocation calls/requested bytes | Instrumented allocation requests during parse | Cumulative requests, not live, retained or operating-system memory. |
| Retained bookkeeping bytes | Trace/query identity storage at its declared high-water boundary | Diagnostic-only overhead; state allocator and included structures. |
| Replacement cost | Rebuild/rehash/snapshot/invalidation/fallback work, only where counted | Null until hooks define and measure it; do not infer from table size. |
| Model rows and weights | Actual accumulation boundary events | Reports observed contributions; repeated row access is not automatically redundant. An alternate scoring implementation is eligible when its observable output is byte-identical. |
| Input/output bytes | Parser-boundary byte counts with exact inclusion rules | Separate from trace serialization and JSONL output dumper bytes. |

For each count-valued metric the analyzer reports per-address values and nearest
rank median, p95 and maximum for each declared stratum. Strata include source,
split, language and country; missing language/country values are `unknown`.
No country-derived tables, thresholds, caches or workload-selected runtime
paths are permitted. Distributions are computed only from genuine per-address
trace rows. Historical experiment 185 aggregate summaries can be included as
legacy context, but cannot be expanded into per-address distributions.

## Separate decisions

Correctness eligibility is an independent byte-exact and single-thread gate.
Candidate architecture acceptance is based on observable output equality,
single-thread execution, throughput, and memory no greater than the preserved
original under the established benchmark protocol. Internal operations and
stage/event counts may differ between architectures; they are diagnostic
guidance, not a correctness gate. Cross-architecture counter comparison is made
only where a mapping is explicitly supported, and missing mappings do not
disqualify an otherwise correct architecture. Diagnostic operation counts do
not pass speed or memory acceptance. This report intentionally has no aggregate
optimality score. Product acceptance still requires the established unmodified
benchmark, cold/warm throughput, setup, peak and steady memory, complete output
preservation, and single-thread evidence against the preserved 000 oracle.

The 2026-10-07 smoke collection exercised four named API cases (London,
SW1A 1AA, empty input, and punctuation-only) through original, release-control,
and diagnostic libraries. Their serialized API outputs were byte-identical;
the diagnostic trace completed all four addresses with 74 query records, no
overflow, and every sampled parser thread count equal to one. This is an
integration smoke result, not a throughput or memory acceptance result. The
edge API harness separately selects only records marked safe-to-call; embedded
NUL and NULL-address cases are correctness-only when the trace boundary cannot
represent their effective C-string input. The 2026-10-07 API parity collection
covered all 78 safe registry records (including those two API-only cases); the
original, release-control, and diagnostic serialized records were byte-identical
with a common SHA-256. All sampled thread counts were one. Four invalid-byte
records were excluded by the harness. Unsafe byte probes remain excluded.
