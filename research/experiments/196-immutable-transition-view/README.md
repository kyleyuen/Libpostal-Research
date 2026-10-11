# Experiment 196 — score Viterbi with the immutable model transition matrix

Date/time/timezone: 2026-10-07 America/Los_Angeles; hypothesis recorded before source edit.
Status: completed implementation attempt; release/helper and corrected caller-inclusive development/golden output/thread/frozen checks passed. It removes 10,305/12,860 transition-matrix copies and 32,976,000/41,152,000 logical read bytes plus the same writes (development/golden). The v1 scalar-read counter is invalid and its v2 correction is source-only. CPI already uses `EXP_TRANS_ALIAS` for this copy avoidance; no novelty over CPI or timing/footprint acceptance is claimed.
Protocol: `research/METHOD.md`.

Benchmark_186 release-only parity supplements passed on development and golden inputs against retained O000 output; golden outputs matched frozen-v1 (12,863 rows), and all sampled roles reported one thread. Raw evidence is under `research/analysis/parallel-exploration-2026-10-07/raw/` with the matching experiment ID and `development-release-parity-supplement1` / `golden-release-parity-supplement1` suffixes. Diagnostic copy-call and payload reductions were measured in v1; speed and memory remain unmeasured.

## Hypothesis and change

The 181 scorer copies the model's 20×20 transition-weight matrix into context-owned storage once per address. In parser inference those weights are immutable. Add `crf_context_viterbi_with_trans`, an explicit read-only optional transition-array argument; the existing `crf_context_viterbi` calls it with NULL and retains the old owned-matrix behavior. The combined `crf_tagger_score_viterbi` path passes `self->trans_weights->values` and skips the copy; standalone `crf_tagger_score` calls still copy into owned context storage exactly as before. The inner Viterbi score loop reads the same double values in the same order. The change removes 3,200 logical bytes read and 3,200 logical bytes written per address. No context field or persistent storage is added.

A read-only review of the initial context-view design found reset and mutability contract risks; those changes were discarded before build. The final API is explicit and const, so the original `crf_context_viterbi` wrapper, trainer, and standalone scorer continue using their own transition matrix.

## Operation-based diagnostics

Count removed transition-matrix copy calls and 6,400 logical payload bytes per address; this is not a hardware/DRAM traffic claim. Context storage remains unchanged.

| Metric | 181 control | Candidate | Evidence |
|---|---:|---:|---|
| matrix copies per scorer call | 1 | 0 | source + benchmark counters |
| output parity / speed / memory | pending | pending | benchmark_186 |

## Reproduction and provenance

Base source: experiment 181 `crf.c` and original libpostal `crf_context.c/.h`. Source manifest, patches, exact builds, timestamps, flags and hashes are in `raw/`. Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.
Parser invoked by implementation author: no.

## Correctness and decision

The synthetic helper compares original and explicit-array Viterbi results using identical and alternate matrices, requiring exact score equality and byte-equal label sequences; it also checks reset of owned transition storage. Parser parity/thread/memory gates remain pending.

Initial release build attempt is preserved at `raw/build_release.*` (exit 1): its script selected the unchanged `address_parser.c`, which had not been copied into the candidate source directory. The exact 181 source is now included in the immutable source tree.

Build attempt 2 then failed because `crf.h` included the original context header before the new Viterbi declaration. `crf.c` now includes the candidate `crf_context.h` first; attempt 2 diagnostics remain preserved.

Build attempt 3 exposed a stale copied 195 compile-check command referencing the trainer source, which 196 had not included. Added the unchanged original trainer source for that compatibility compile check; failure output remains in `raw/build_release_attempt3.*`.

The successful release/helper build is `raw/build_release_attempt4.*`; earlier integration failures are preserved in attempts 1–3.

## Corrected diagnostic results

The all-caller derivative passed development/golden output and one-thread checks; golden matched frozen-v1. It avoided 10,305 / 12,860 transition-matrix copies. The logical payload avoided was 32,976,000 bytes read and 32,976,000 written in development; golden avoided 41,152,000 read and 41,152,000 written. The v1 diagnostic counter reported 402,304,000 / 502,368,000, but that counter incremented by L=20 at each innermost scalar read and overstated reads by 20×. Retire those figures as read counts. Source-derived actual scalar reads are 20,115,200 development and 25,118,400 golden (the former counts divided by 20); copy calls and logical copy-payload read/write counters above are unaffected. A fresh source-only diagnostic revision v2 changes the counter to +1 per scalar load; it has not been built or captured. Allocation, lookup, model-row, and weight counters matched 181. These are source-level bytes, not physical DRAM traffic; timing and RSS remain unmeasured. The v1 captures remain preserved at `research/analysis/parallel-exploration-2026-10-07/raw/196-{development,golden}-diagnostic-supplement3/`; their copy metrics remain valid, but the transition-cell read counter is invalid. Source-only corrected counter revision and hashes: `research/analysis/parallel-exploration-2026-10-07/raw/196-diag-v2-source-handoff.json` and `196-diag-v2-source-manifest.sha256`.

## Final attempt audit (2026-10-08)
This section supersedes earlier pending statements in this record, which predated the final captures.


This numbered attempt is complete. The valid all-caller diagnostic captures passed development/golden output/thread/frozen checks and counted 10,305/12,860 avoided transition-matrix copies, with 32,976,000/41,152,000 logical bytes avoided in each read and write direction. Source-derived actual scalar transition reads are 20,115,200/25,118,400; the v1 counter overstated them 20× and is retired. The v2 counter fix is source-only, unbuilt, and not evidence of a new capture. This overlaps CPI's existing `EXP_TRANS_ALIAS` mechanism, which aliases model transitions instead of copying them; the 196 explicit const-argument API is a different interface, not a new copy-avoidance result against CPI. No speed or process-memory gate was run. Failed build attempts and invalidated counter captures remain preserved in `raw/`.
