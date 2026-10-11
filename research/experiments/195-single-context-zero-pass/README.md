# Experiment 195 — remove the duplicate CRF context matrix clear

Date/time/timezone: 2026-10-07 America/Los_Angeles; hypothesis recorded before source edit.

## Corrected diagnostic results

The corrected caller-inclusive derivative (crf.c and crf_context.c) passed development/golden output and one-thread checks; golden matched frozen-v1. It recorded 20,610 omitted pre-reset matrix-zero calls and 203,592,480 logical zero-store bytes avoided on development, and 25,720 calls / 254,204,160 logical bytes on golden. Model rows, weight additions, allocation counts/bytes, and lookup counts remained equal to 181. These bytes count logical array elements skipped by the helper path; the required subsequent reset still clears them, so this is not DRAM traffic or net memory reduction. Raw metrics are `research/analysis/parallel-exploration-2026-10-07/raw/195-{development,golden}-diagnostic-supplement2/`. Speed and RSS remain unmeasured.

Protocol: `research/METHOD.md`.

Benchmark_186 release-only parity supplements passed on development and golden inputs against retained O000 output; golden outputs matched frozen-v1 (12,863 rows), and all sampled roles reported one thread. Raw evidence is under `research/analysis/parallel-exploration-2026-10-07/raw/` with the matching experiment ID and `development-release-parity-supplement1` / `golden-release-parity-supplement1` suffixes. Diagnostic metric runs are queued; no operation, speed, or memory change is yet measured.

## Hypothesis and change

`crf_context_set_num_items` clears the alpha, beta, state, and state-transition matrices. Both in-repository callers—`crf_tagger_score` and the averaged-perceptron trainer—immediately follow a successful resize with `crf_context_reset(..., CRF_CONTEXT_RESET_ALL)`. That reset clears state, state-transition, and transition matrices again. The first state and state-transition clears are redundant full matrix writes.

Keep the existing `crf_context_set_num_items` behavior intact. Add a caller-specific `crf_context_set_num_items_for_reset` path that skips only the state and state-transition clears, then use it at both in-repository call sites that immediately reset all scores. Keep alpha/beta clears (the reset routine does not clear them), backward-edge initialization, and the later reset of state/state-transition/transition matrices. No storage or numeric values change before the reset.

## Operation-based diagnostics

Required work metric: logical bytes assigned zero in the state and state-transition matrices by the pre-reset resize path. The candidate removes `T * L * sizeof(double) + T * L * L * sizeof(double)` such logical stores per successful preparation; the following reset still performs its required clears. This is a source-level logical-store count, not a hardware/DRAM traffic claim. Confirm the two callers and output behavior. Counter changes belong in the isolated diagnostic derivative.

| Metric | 181 control | Candidate | Evidence |
|---|---:|---:|---|
| Redundant zero bytes per sequence | pending | structurally zero | benchmark_186 diagnostics |
| Output parity / speed / memory | pending | pending | benchmark_186 |

## Reproduction and provenance

Base source: `research/experiments/181-release-180/sources` plus the original `libpostal/src/crf_context.c` and `.h`. Exact source hashes, patch, commands, flags, outputs and artifact hashes are in `raw/`. Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.
Parser invoked by implementation author: no.

## Correctness and decision

A repository-wide search found exactly two callers; each uses the caller-specific path then resets all matrices immediately after successful resizing and before feature scoring. The public existing setter retains its zero-on-set behavior. `crf_context_reset` does not clear alpha, beta, or backward-edge matrices, so the helper keeps their initialization. The synthetic context self-check exercises the unchanged setter and the new resize/reset path, including a resize. Byte-identical output and one parsing thread remain hard gates.

Build attempt 1 is preserved at `raw/build_release.*` (exit 1): `crf.c` saw the original context header through `crf.h`, not the candidate declaration. Candidate `crf_context.h` is now included before dependent headers in both changed source files. Source review by new_architecture found no reset/caller blocker.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. The corrected derivative includes changed caller TUs and passed development/golden output/thread/frozen checks. It omits 20,610/25,720 pre-reset zero calls and 203,592,480/254,204,160 logical element-store bytes in development/golden; the following mandatory reset still clears those same matrices, so these counts do not establish net physical stores avoided or memory reduction. This overlaps CPI's `EXP_NO_RESET` goal of avoiding redundant context initialization, but the mechanics differ: CPI's setter clears required matrices and its scorer skips reset, whereas this helper skips the setter's duplicate zeroing before reset. No timing or footprint gate was run. Initial build and derivative failures remain preserved in `raw/`.
