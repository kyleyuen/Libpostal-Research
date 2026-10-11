# Experiment 189 — bounded stack key for direct state-word lookup

Date/time/timezone: 2026-10-07 America/Los_Angeles; pre-registered before source edit.
Status: completed implementation attempt; release build and development/golden output/thread/frozen checks passed. The stack-key route removes 59,186/73,873 allocation calls and 682,160/851,126 requested bytes (development/golden); key construction and trie searches remain. Timing and process-memory acceptance are unmeasured.
Protocol: `research/METHOD.md`.

## Hypothesis and change

In 181, every recognized token that can take the direct state-word route calls `crf_state_word_id`, which allocates `strlen(word)+6` bytes, writes the constant prefix plus the word, searches the model trie, then frees the buffer. A fixed 256-byte stack buffer should remove that allocation pair for normalized words up to 250 bytes. Longer words retain the exact heap allocation path as a general structural fallback. Key bytes, lookup semantics and miss behavior stay unchanged.

Exact implementation: `crf_state_word_id` uses `char stack_key[256]`, then mallocs only when `n+6` exceeds that capacity. It does not cache results or depend on corpus identity.
Base: Git commit `181737d30977adc9d287e3f7a1cfafd7277525d`; base source hashes in `raw/base_hashes.sha256`.

## Operation-based research diagnostics

Predicted changes: reduce instrumented parser allocation calls by one per qualifying direct word; requested heap bytes decrease by `n+6` each. Trie lookup calls and key bytes copied stay constant; long words use the old allocation semantics.
Metrics: allocator calls and requested bytes from `diag_alloc` wrappers; direct ID offered/consumed; state lookup calls/hits/misses; stack versus heap key path counts (add diagnostic-only counters in the isolated 185 derivative if needed). Scope is the complete parser call; denominator is addresses and qualifying direct word IDs.
Diagnostic build and matching unchanged source control: delegated to benchmark_186 using 185 `prototype181` hooks and the exact patch, including diagnostic-only stack/fallback counters. Release is built here from 181 sources; build exit 0, 2026-10-07 19:00:36–19:00:45 PDT.

| Metric | 181 diagnostic control | Candidate | Evidence |
|---|---:|---:|---|
| Instrumented allocation calls, dev/golden | 69,508 / 86,751 | 10,322 / 12,878 | reductions of 59,186 / 73,873 |
| Requested allocation bytes, dev/golden | 934,944 / 1,166,110 | 252,784 / 314,984 | reductions of 682,160 / 851,126 B, entirely from `crf_alloc` |
| Lookup calls, dev/golden | 848,149 / 1,058,689 | unchanged | all word-key lookups remain |
| Model rows / weight additions | 483,550 / 11,261,655 dev; 603,376 / 14,045,912 golden | unchanged | exact counters |

Development and golden byte/thread/frozen gates passed. The 185 allocation breakdown attributes requested bytes to `parser_alloc` and `crf_alloc`: parser remains 251,760 / 313,960 B, while CRF drops from 683,184 / 852,150 B to 1,024 B. These instrumented counters show removed temporary-key heap requests; they do not measure process RSS or speed. Per-capture counters and checks are preserved under `research/analysis/parallel-exploration-2026-10-07/raw/189-{development,golden}-diagnostic-supplement1/`.

## Reproduction

Hardware/OS/compiler: Apple Silicon macOS, Apple clang; flags captured in `raw/build_flags.txt` after compile.
Exact build commands, stdout/stderr/exit/timestamps: `raw/build_release.*`; source patch and artifact hashes: `raw/source_manifest.sha256` and `raw/final_hashes.sha256`.
Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad` (`benchmark/model_sha256.txt`). Parser invoked by implementation author: no.

## Correctness and interpretation

Exact key content is unchanged. Original output, one-thread, timing and memory evidence are pending benchmark_186. The size boundary is an implementation buffer size, not an input-derived threshold. Fallback behavior remains for arbitrarily long tokens, subject to the existing allocation failure semantics.

## Decision

Release build succeeded; benchmark-agent evidence pending.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. The successful serialized development/golden diagnostic captures report allocation-call reductions of 59,186/73,873 and requested-byte reductions of 682,160/851,126; parser lookup counts remain 848,149/1,058,689. This replaces the earlier pending diagnostic wording above. The optimization removes temporary heap requests for keys within the fixed stack buffer while retaining key construction, the same trie search, and heap fallback for longer keys. It does not establish reduced execution time or process footprint. The lock-gate failures and successful retry captures remain in `raw/`.
