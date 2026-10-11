# Experiment 192 — reuse front-record vocabulary frequency for neighbors

Date/time/timezone: 2026-10-07 America/Los_Angeles; hypothesis recorded before source edit.
Status: completed implementation attempt; release build and development/golden six-role output/thread/frozen checks passed. It removes 47,346/59,107 lookup calls with zero guard fallbacks and unchanged allocations/model rows/weights. Its guarded neighbor-frequency reuse overlaps CPI's per-address vocabulary memo strategy; it is not a new cache family relative to CPI. No timing or footprint acceptance is measured.
Protocol: `research/METHOD.md`.

Verified development/golden lookup, repeat, model-row, weight-addition, allocation, output/thread, and frozen-reference results are recorded in `research/analysis/parallel-exploration-2026-10-07/REPORT.md`, Experiment 192. These are operation-count and parity results; no speed or RSS promotion claim is made.

## Hypothesis and change

The 181 front fill already performs one word-vocabulary lookup per normalized token and stores its exact frequency in an index-aligned per-address front record. During feature generation, each token also asks the vocabulary for its previous and next normalized word when that neighbor is an ordinary, prefix, or suffix word. `word_or_phrase_at_index` returns the same normalized token string for these plain-word cases. Reusing the already-computed frequency for those exact indexes can remove repeated vocabulary searches, without adding storage or changing unknown-word handling. If the front array was unavailable, retain the existing lookup fallback.

This applies uniformly by phrase type and address position, not by content. The words remain looked up once during front fill. Phrase and component phrase behavior is untouched. The candidate uses only per-address front records and introduces no cross-address cache.

## Operation-based diagnostics

Expected exact work removed: one query for each in-range neighboring index that resolves to a plain-word phrase type while a matching front record exists. Required instrumentation: vocab lookup counts by stage and plain-neighbor cache hits by previous/next side; front-record fallback count; allocations/bytes and output parity. Diagnostic additions must be isolated from release source. Benchmark agent owns all parser/model runs.

| Metric | 181 control | Candidate | Evidence |
|---|---:|---:|---|
| Vocabulary searches | pending | pending | benchmark_186 |
| Neighbor frequency reuses | pending | pending | benchmark_186 |
| Output parity / speed / memory | pending | pending | benchmark_186 |

## Reproduction and provenance

Exact source hashes, patch, commands, flags, logs, binary/model hashes are in `raw/`. The shared-lock release build ran 2026-10-07 PDT; command and timestamp files are `raw/build_release.*`, exit 0. Release artifacts and immutable descriptor are in `raw/handoff.json`. Base source is `research/experiments/181-release-180/sources`; model manifest SHA-256 is `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.
Parser invoked by implementation author: no.

## Correctness and decision

The cache lookup is valid only when the phrase helper's result is backed by the normalized token at the same index. Source review confirms null/prefix/suffix results return `cstring_array_get_string(context->normalized, i)`; all other phrase types continue using the original path. Allocation failure leaves `ap_front_n` zero and must preserve existing lookup behavior. Output parity, single-thread behavior, and original peak/steady footprint remain hard gates. Decision pending build and serialized benchmark.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. Development/golden diagnostic captures show 47,346/59,107 fewer physical lookup calls (26,571/33,179 previous-neighbor and 20,775/25,928 next-neighbor reuses), zero guard fallbacks, and unchanged rows, weights, and allocation counts/bytes; output and one-thread checks passed. This overlaps the existing CPI `EXP_VOCAB_MEMO` per-address token-frequency memo strategy. The 187 diagnostic report counted 546,206 existing token-memo answer reuses across all13; the candidate's golden-v1 count is not additive evidence against packaged CPI. No direct packaged-CPI operation-count comparison, speed result, or memory gate is established here. Failure/retry logs remain in `raw/`.
