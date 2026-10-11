# Experiment 194 — use known key lengths for trie lookups

Date/time/timezone: 2026-10-07 America/Los_Angeles; hypothesis recorded before source edit.

## Corrected diagnostic results

The corrected all-caller derivative (address_parser.c, crf.c, trie.c, diag.c) passed development/golden output and one-thread checks; golden matched frozen-v1. On development/golden, `trie_get_data_len` served 706,341 / 881,645 calls and the instrumented key-length proxy counted 12,892,934 / 16,099,269 bytes of former `strlen` scan work avoided. Total lookup calls stayed at 848,149 / 1,058,689, and model rows, weight additions, allocations/bytes were unchanged. The repeat counter is lower by two (113,742 vs 113,744 development; 142,239 vs 142,241 golden); this identity discrepancy is unresolved, so no search-reduction claim is made. Raw metrics are `research/analysis/parallel-exploration-2026-10-07/raw/194-{development,golden}-diagnostic-supplement2/`. Length-scan bytes are a structural proxy, not cycles. Speed and RSS remain unmeasured.

Protocol: `research/METHOD.md`.

Benchmark_186 release-only parity supplements passed on development and golden inputs against retained O000 output; golden outputs matched frozen-v1 (12,863 rows), and all sampled roles reported one thread. Raw evidence is under `research/analysis/parallel-exploration-2026-10-07/raw/` with the matching experiment ID and `development-release-parity-supplement1` / `golden-release-parity-supplement1` suffixes. Diagnostic metric runs are queued; no operation, speed, or memory change is yet measured.

## Hypothesis and change

The prototype181 diagnostic baseline records more than one million trie/API lookups on golden-v1. The current `trie_get_data` path calls `strlen` to discover each key's length, then trie traversal reads the key again. Existing `cstring_array` entries already store token starts, so their exact byte lengths are available in O(1); normalized token/subword callers also already carry lengths. This experiment adds a length-aware trie data lookup that traverses exactly `len + 1` bytes, including NUL, and uses known lengths at feature, transition, vocabulary, and direct state-word lookup sites.

All lengths come from the same C string storage passed to the original lookup. If a stored length is unavailable, the existing NUL-terminated lookup remains the fallback. This changes no query identity, storage, event order, result multiplicity, or cross-address state.

## Operation-based diagnostics

Required counters: actual lookup calls by family; key bytes whose preliminary `strlen` scan is eliminated; fallback count for unavailable lengths; trie edge and compressed-tail bytes; allocation/copy bytes and output parity. Candidate traverses the same bytes and eliminates the preliminary full-key length scan for length-aware lookups, plus a second suffix scan for compressed tails on all trie lookups. No speed prediction is recorded as measured.

| Metric | 181 control | Candidate | Evidence |
|---|---:|---:|---|
| Lookups with `strlen` pass avoided | pending | pending | benchmark_186 |
| Length-fallback calls | pending | pending | benchmark_186 |
| Output parity / speed / memory | pending | pending | benchmark_186 |

## Reproduction and provenance

Base source: `research/experiments/181-release-180/sources` plus untouched `libpostal/src/trie.c` / `trie.h` replaced by this candidate. Exact source hashes, patch, commands, flags, outputs and artifact hashes are in `raw/`. Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.
Parser invoked by implementation author: no.

## Correctness and decision

The new API uses existing `trie_get_len`, which follows the same transitions and compares the terminating NUL. The candidate also replaces compressed-tail `strlen` with the exact remaining query length, including NUL; this benefits both explicit-length and existing NUL-terminated lookups. Array token lengths are derived from stored offsets; direct string lengths are already computed by the caller. Every modified source path must preserve exact lookup IDs and misses, including UTF-8 byte lengths and compressed tails. Byte-identical outputs and one parsing thread remain hard requirements. Decision pending build and serialized benchmark.

Build integration attempts and compiler diagnostics are preserved in `raw/build_attempts.md`; the successful build is attempt 7. The source-reviewer confirmed exact byte-length/tail handling; API requires the supplied length to equal the C-string byte length excluding NUL.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. The corrected all-caller derivative passed output/thread/frozen gates. It avoided the preliminary key-length scans measured by the stated structural proxy, not physical searches; total lookup counts stayed 848,149/1,058,689. The repeat counter differs from control by two in each corpus, an unresolved identity artifact, and is not claimed as search reduction. Speed and process-memory gates were not run. Earlier conflicting derivative attempts and the corrected caller-inclusive captures remain preserved in `raw/`; the accepted metrics are specifically from diagnostic supplement 2.
