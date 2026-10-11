# Experiment 191 — traverse a segmented state-word key

Date/time/timezone: 2026-10-07 America/Los_Angeles; hypothesis recorded before source edit.
Status: completed standalone implementation attempt; release build, expanded helper, and development/golden output/thread/frozen checks passed. It removes 59,186/73,873 temporary joined keys and 682,160/851,126 requested allocation bytes (development/golden), with physical lookup/repeat totals unchanged. No standalone full13, timing, or footprint gate was run.
Protocol: `research/METHOD.md`.

## Hypothesis and change

181's direct-ID lookup builds `word|TOKEN` into temporary contiguous storage, copying five constant prefix bytes and every token byte before calling the trie. 189 tests removing the heap allocation but retains those copies. This experiment changes the trie traversal API to walk prefix and suffix segments directly, including exact comparisons for compressed tails, so the direct state-word route performs the same dictionary lookup without materializing the joined key.

Exact change: add a length-delimited segmented-key data lookup to trie.c/trie.h and use it only for the constant `word|` prefix plus normalized word. The regular `trie_get_data` API remains unchanged. No cache or input-derived path. Helper-level checks compare the standard full-key API and segmented API for byte-identical result/value behavior across matching terminals, misses, UTF-8 and compressed tails; the helper is not a parser/model run.
Base: commit `181737d30977adc9d287e3f7a1cfafd7277525d`; copied source hashes in `raw/base_hashes.sha256`.

## Operation-based research diagnostics

Predicted work: eliminate `n+5` bytes of prefix/key writes per eligible direct-ID lookup and the temporary key allocation; add per-character segment selection branches during trie traversal. Trie transition count and result semantics are intended to remain the same. The tail-compressed branch must compare every remaining joined byte and terminating NUL exactly.
Metric units/coverage: direct-key bytes materialized (control) vs segmented bytes copied (candidate), trie transition/probe count, tail-comparison bytes, allocation calls/bytes, exact lookup identity. Diagnostic-only counters belong in benchmark_186's isolated derivative; calibrate segmented comparator for full-key and tail-key hits/misses.

| Metric | 181 control | Candidate | Evidence |
|---|---:|---:|---|
| Temporary `word|TOKEN` constructions, dev/golden | 59,186 / 73,873 | 0 / 0 | exact diagnostic counts |
| Temporary key bytes, dev/golden | 622,974 / 777,253 | 0 / 0 | excludes NUL; allocation includes NUL |
| Allocation calls, dev/golden | 69,508 / 86,751 | 10,322 / 12,878 | −59,186 / −73,873 calls |
| Requested allocation bytes, dev/golden | 934,944 / 1,166,110 | 252,784 / 314,984 | −682,160 / −851,126 B |
| Trie transition probes / tail bytes | unavailable / unavailable | unavailable / unavailable | parser hooks do not expose them |
| Structural full/segmented key identity | 6 hits / 5 misses | identical IDs and hit/miss results | expanded helper, 11 cases |
| Output parity | pending | passed dev + golden six-role; frozen golden matched | benchmark_186; speed/memory still pending |

## Reproduction

Apple clang and exact flags: `raw/build_flags.txt`; exact compiler command/output/status/time: `raw/build_release.*`; source patch/hash manifest in `raw/`.
Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.
Parser invoked by implementation author: no.

## Correctness and interpretation

This duplicates a small portion of the existing double-array trie traversal and must preserve its compressed-tail semantics, including the NUL terminator. Development and golden output/thread parity passed, and the golden frozen comparison passed. The diagnostic screen found no lookup/repeat reduction; it did eliminate 59,186 development and 73,873 golden temporary word-key constructions, with lower instrumented allocation counts. Timing and memory promotion remain pending. See the full measured table in `research/analysis/parallel-exploration-2026-10-07/REPORT.md`, Experiment 191.

## Decision

Development and golden correctness screens passed; structural helper passed all eleven cases. The query counts are unchanged; temporary joined keys and their instrumented allocations are removed. Timing and process memory remain unmeasured, so speed/memory promotion is not established.

Build note: first compile attempt failed before producing artifacts because `crf.h` resolved libpostal's original `trie.h` before the candidate declaration; the exact failure remains under `raw/build-attempt1/`. The retry explicitly includes the candidate trie header first and built successfully in `raw/build_release.*`, exit 0 at 19:15:12–19:15:21 PDT. The first helper revision passed with 3 hits and 4 misses. Its expanded source now checks six hits and five misses: shared-prefix terminal, segment crossing inside a compressed tail, empty prefix, UTF-8 boundary, long compressed tail, empty suffix, and exact/mismatched misses. The expanded test built and ran under the shared workload lock with exit 0. Full-key and segmented result IDs/values agreed across all 11 checks; runtime output is in `raw/selfcheck/run.stdout`.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This standalone numbered attempt is complete. The expanded structural helper passed 11 hit/miss cases; diagnostic development/golden captures report 59,186/73,873 fewer temporary joined keys and 682,160/851,126 fewer requested allocation bytes, while lookup calls and canonical repeated-query counts match 181. The all13 screen belongs to the composed experiment 197, not standalone 191. No timing or process-memory result exists. The initial include-order build failure and successful candidate-header retry, plus the earlier helper revision, remain preserved in `raw/`.
