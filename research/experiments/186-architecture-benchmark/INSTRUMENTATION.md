# Experiment 186 trace instrumentation

## Purpose and source identity

186 adds a streaming, per-address trace to an isolated copy of
185-diagnostic-instrumentation/source/original. It instruments the copied
original parser and dictionary hooks only. It does not change a production
candidate or claim a performance improvement. source/diag.c,
source/original/diag.h, and the modified source/original/address_parser.c are
specific to 186; experiment 185 remains untouched.

tools/build_trace.sh --build compiles two same-source siblings:

- original-control: copied original source with diagnostic macros compiled out.
- original-trace: the same copied parser plus 185 diagnostic sites and the
  186 streaming writer.

Both link tools/dump_outputs.c. The control build is useful for same-source
output comparison. The preserved 000 baseline remains the behavioral reference.
The command writes timestamped stdout, stderr, command, exit, build-flag, source
hash, and artifact-hash records below raw/build-<id>/; binaries are below
artifacts/build-<id>/. No parser is run by the build command.

Build-time model path defaults to
/Users/kyle/.cache/libpostal-profile/libpostal and can be overridden with
LIBPOSTAL_MODEL_DIR. The copied source tree is the 185 instrumented original
copy. Its hash is recorded separately from new 186 files.

## Collection interface

Run the trace dumper with one newline-delimited input file. Set:

- TRACE_JSONL: output path for the JSONL trace.
- TRACE_CASE_ID: caller-defined case identity.
- TRACE_EXPECTED_ADDRESSES: expected number of nonempty input lines.
- TRACE_EVENT_CAP: optional positive cap on query records; defaults to
  1,000,000.

The dumper's stdout remains ordered output JSONL in the 182 format. stderr,
stdout, trace JSONL, and the input file should be retained independently.
diag_trace_finish() returns failure if the trace overflows or observed address
count differs from the declared expectation.

## Record definitions

Each trace starts with one trace_header. It declares schema version, case
identity, expected address count, event cap, exact query-identity fields, and
known limitations.

Each address receives a stable 1-based index in input order. Its address key
is SHA-256 of exact input bytes excluding line terminators. The original
address text is not copied into the trace.

Every observed hook writes one query record immediately, then releases its
temporary key buffer. query_index is the per-address order of all logical
requests seen at instrumented sites. Physical searches also receive a
per-address physical_search_index. An explicit diag_reuse hook is a logical
answer-reuse event with no physical search index. Each record contains the
dictionary pointer identity (process-local), operation, family, request stage,
hit/miss result, and full canonical query bytes as lowercase hex.
diag_lookup_parts keys include two native-endian uint64 part lengths followed
by both byte strings, matching 185's exact query-identity key.

When exact dictionary pointer, operation, family and canonical bytes have
appeared earlier in the same address, producer_stage and
producer_query_index refer to the first observed event. Otherwise both are
null. This is an observed identity/repetition link; it does not establish value
flow or prove reuse safety.

One address_summary follows each parse and provides per-address physical and
logical request counts, hits/misses, scoped allocation requests, scorer key
materialization, model-row and weight counts, output/input byte counts,
diagnostic tracker bytes, and the number of key bytes written. Allocation
sites and other counters retain 185's source coverage; they are not global
libc totals.

trace_end gives observed and expected address counts, query event count,
serialized key-byte count, and completion status. If the query-record cap is
reached, one trace_overflow record is written, collection stops adding query
records, and the process exits nonzero. Downstream tools must reject the
entire run for complete-trace claims. Address summaries after overflow contain
lower bounds and remain explicitly incomplete.

## Completeness and interpretation

The following values are deliberately unknown:

- Dictionary generation is null. The original dictionary APIs expose no
  reliable mutation/reset generation. Pointer reuse across destroy/recreate
  cannot be distinguished.
- Safe-reuse proof is unknown. diag_reuse is an explicit source hook, but the
  trace does not establish the cached answer's validity conditions.
- Producer/request linkage is an exact repeated-query observation. No complete
  semantic value-flow or end-to-end reuse proof is claimed.

Query coverage includes lookup sites instrumented in the copied 185 original:
trie key, phrase-token, feature hash/integer and vocabulary canonicalization
hooks. It is not every lookup inside the library. The trace does not count hash
probes or trie edges, and does not capture scorer text copies beyond key
materialization at the existing 185 scorer boundary. Allocation counts/bytes
cover only already instrumented parser and CRF source bodies. Model rows/weights
cover their existing CRF accumulation boundaries. Tracker byte values describe
185 diagnostic identity/pattern storage, not allocator metadata or trace-file
filesystem cache.

Keys and dictionary addresses can contain sensitive internal query material
and are process-specific. Keep trace artifacts with experiment records. This
is explanatory instrumentation only; do not time it as a performance build.
Parsing remains one-threaded.
