# Experiment 193 — carry an exact known-word CRF miss into scoring

Date/time/timezone: 2026-10-07 America/Los_Angeles; hypothesis recorded before source edit.
Status: completed implementation attempt; release build, development/golden output/thread/frozen checks, and packaged CPI output supplements passed. It skips 2,817/3,509 exactly confirmed duplicate-miss searches (development/golden); event slots, rows, weights and allocations are preserved. No timing or footprint acceptance is measured.
Protocol: `research/METHOD.md`.

Verified development/golden lookup, repeat, model-row, weight-addition, allocation, output/thread, and frozen-reference results are recorded in `research/analysis/parallel-exploration-2026-10-07/REPORT.md`, Experiment 193. These are operation-count and parity results; no speed or RSS promotion claim is made.

## Hypothesis and change

In 181 front-fill, every known-vocabulary normalized token gets an exact lookup of the state feature key `word|TOKEN`. When that key is absent, feature generation later emits the same `word|TOKEN` string and the CRF scorer repeats the same state-feature trie search. Golden-v1 has 3,509 direct-word fallback events, an upper bound on the opportunities relevant to this path; it is not a speed claim.

This candidate adds an explicit confirmed-miss status distinct from a failed key allocation. It carries that status to the exact `word` feature slot and suppresses only the duplicate state-feature lookup. The normal feature string remains emitted, event order and multiplicity remain intact, and no score row is applied for a miss. Allocation failure, a valid ID that cannot be encoded, pointer/index mismatch, or any unconfirmed case uses the original fallback. Status uses no extra per-token storage; any reserved marker is accepted only after checking it cannot collide with the actual returned feature ID for the loaded model. No key, country, or corpus special case.

## Operation-based diagnostics

Required counters: confirmed front-pass misses, duplicate state feature searches skipped, fallback/unconfirmed cases, and event multiplicity/score rows; output and threading parity. A source-derived counter is insufficient; benchmark diagnostics must confirm. Parser/model runs belong to benchmark_186.

| Metric | 181 control | Candidate | Evidence |
|---|---:|---:|---|
| Repeated known-word miss lookups | pending | pending | benchmark_186 |
| Score contribution multiplicity | pending | unchanged by design; verify | benchmark_186 |
| Output parity / speed / memory | pending | pending | benchmark_186 |

## Reproduction and provenance

Base source: `research/experiments/181-release-180/sources`. Exact source hashes, patch, commands, flags, outputs and artifact hashes are in `raw/`. Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.
Parser invoked by implementation author: no.

## Independent source review

`new_architecture` reviewed the source delta on 2026-10-07 and found the byte key and state-feature trie identical between front lookup and later scorer lookup. The normalized word remains unchanged in the known-word branch. Reserved ID `UINT32_MAX-1` is accepted only when the completed lookup misses; a hit at or above the marker remains on the original path. The marker retains the feature at its original index and skips only the redundant query. No parser/model run was part of the review.

## Correctness and decision

The initial lookup and later scorer lookup must be byte-for-byte the same key and use the same loaded CRF state-feature trie. The current branch is restricted to known vocabulary words whose normalized token string is passed unchanged to the `word` feature builder. Only a completed trie miss may suppress the duplicate; allocation failure or an ID at/above the reserved sentinel falls back. A loaded-model ID collision is checked at runtime. The candidate retains the emitted feature and its position, so it removes no feature occurrence and applies no score for an absent key. Any uncertainty or failed gate rejects the optimization. Byte-identical golden output and one parsing thread remain hard requirements.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. Corrected diagnostic captures count 17,900/22,299 front-word misses and 2,817/3,509 attached markers/skipped duplicate scorer searches in development/golden, with zero fallbacks. The ordinary feature occurrence remains at its original slot; row/weight and allocation counts are unchanged. Release and diagnostic outputs matched the retained original and frozen golden, with one thread; the separate packaged CPI release output supplement also passed. No timing or memory gate was run. The initial CLI-selection failure and all later successful captures are retained in `raw/`.
