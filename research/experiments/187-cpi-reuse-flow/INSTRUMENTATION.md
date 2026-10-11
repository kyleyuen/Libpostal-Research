# Experiment 187 instrumentation

## Build and collection boundary

The build tool has a preflight mode that validates files/tools without compiling
or loading a model. A build creates a matching CPI release-control and
diagnostic pair from this experiment's frozen copy under source/cpi/. Both use
the exact base, CPI, and 119 delivery flags recorded in
research/final/119-cpi/switches.txt; the diagnostic adds only -DEXP185_DIAG.
The build records compiler commands/exits, source hash lists before/after,
flag files, model manifest verification, and artifact hashes under unique
raw/build-* directories. The 000 baseline is not rebuilt or modified.

For a parser collection, set TRACE_JSONL to a new trace path,
TRACE_CASE_ID to the case name, TRACE_EXPECTED_ADDRESSES to the expected
number, TRACE_SOURCE_MANIFEST_SHA256 to the SHA-256 of the build's
source-before.sha256 file, and TRACE_EVENT_CAP to the desired positive cap.
The trace is created exclusively, so an existing path fails. The event cap
counts query and query_consumer rows together. Cap or bounded identity-table
overflow makes the trace incomplete and finalization fails. The dumper also
fails unless parsed-address count matches the declared count.

Standalone event calibration does not load libpostal or model data. Run it as:
bash tools/run_trace_event_calibration.sh <unique-id>

## Event identity and counters

JSONL records follow trace-schema.json. Query indices increase per address.
physical_search means an actual instrumented table/trie probe. answer_reuse is
emitted only at a source-identified memo/dedup boundary and carries the exact
producer query index. precomputed_id marks setup-produced/direct IDs and is
excluded from answer-reuse counts. Physical results separate hit status from
the numeric result, so a frequency of zero is not a miss. A typed vocabulary
frequency miss is recorded as hit=false with value zero because that is the
scalar returned to feature construction; misses with no defined scalar stay
null. Keys are lowercase hex of the exact bytes passed to the instrumented
operation; split feature-dictionary keys use the hook's exact structured
encoding.

query_consumer rows are emitted only when a source query index is carried to
an immediate instrumented consumer. Feature-ID consumers attach each
state/transition score-row occurrence; vocabulary consumers mark the
frequency return used during feature construction. Consumers include their
destination dictionary pointer and result value. A matching numeric ID alone
is insufficient: query identity also includes dictionary pointer,
operation/family, and exact key. The analyzer must check every consumer row.

Numeric dictionary and model generations remain unknown. The header separately
states source proof cpi-within-parse-readonly-v1: dictionaries/model are
read-only during one synchronous, already-set-up address_parser_parse
invocation. This supports only within-parse stability; cross-address reuse,
setup reloads, concurrency, and reentrancy are out of scope. It is not a
numeric generation counter.

## Coverage boundaries and known gaps

The trace follows actual CPI hooks and is deliberately incomplete:

- Vocab frequency hooks include fd_lookup and the CPI pointer-matched token
  memo fill/use. Trie fallback searches are recorded at the result boundary,
  not inside trie edges.
- Address/model phrase wrappers use stable call-site IDs and restore the prior
  site label after each search. Direct/precomputed-ID trace labels also restore
  the caller's previous site.
- State/transition feature IDs are traced at direct dictionary resolution and
  attached to score-row occurrences where the query index is retained.
- Setup role/constant/component phrase IDs are tagged precomputed_id; they are
  not counted as physical or answer-reuse searches.
- Phrase-search hooks do not preserve full ordered phrase payloads; predicate
  hits do not prove phrase lists/extent/data match. Transition/phrase
  consumers without query-index carry, unresolved integer routes, full
  hash-probe/trie-edge counts, and some alternative/batched branches are
  explicitly outside consumer-complete coverage.
- Per-address summaries always state consumer_complete:false. Do not infer
  complete consumer coverage, end-to-end safe reuse, score-addition savings,
  or parser memory from these hooks.
- Diagnostic bookkeeping bytes, trace-key bytes, and requested parser
  allocation counters include instrumentation overhead; they are not control
  build performance or peak-RSS results.

The trace schema makes unknown identities/generations explicit rather than
joining answers by key text. Output equality and benchmark acceptance are
separate gates; build and calibration do not run parser smoke or full-corpus
timing.
