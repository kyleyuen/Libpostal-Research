# Experiment 181 — release-mode 180 event scorer

Date/time/timezone: 2026-10-06T01:05:00-0700 PDT. Pre-registered before release-source creation and build.
Status: release build and targeted semantic/thread smoke passed; no performance measurement.

## Hypothesis and change

Build experiment 180's carried `word|token` ID path with `EXP_EVENT_FLOW` and `EXP_FUSED_FRONT` enabled, while compiling without `EXP_ORACLE` and `EXP_EVENT_STATS`. The direct event remains in original order; oracle file production and runtime counters are absent from this release candidate.

The candidate source starts from the archived 180 sources, not cpi. The sole release-source adjustment gates oracle-only fields and optional statistics so they do not execute or compile in the candidate. Every unhandled word retains the original string and trie-lookup fallback.

## Required evidence

- Build release `libpostal.a`, `dump_outputs`, `bench_ext`, and `fuzz_pipeline` with the same O2/static-library harness style used by 180 and the original baseline comparison.
- Compare release output directly with a dumper linked to preserved `000-baseline/artifacts/libpostal.a` on golden-v1, novel, and OSM v3.
- Capture a live `ps -M` thread observation during a model-loaded candidate parse.
- Retain source copies, patches, flags, commands, timestamps, stdout, stderr, exit statuses, and hashes under this directory. This experiment performs no timing benchmark.

## Build and artifacts

The release build completed with exit 0 in `raw/build.*`. It uses `-O2` for library and harnesses, `-O0` for the scanner, system allocator, static `libpostal.a`, and only `-DEXP_FUSED_FRONT -DEXP_EVENT_FLOW`; exact flags are in `raw/build_flags.txt`. `EXP_ORACLE` and `EXP_EVENT_STATS` are absent. The release adjustment in `raw/release_instrumentation.patch` keeps oracle fields and counters behind those disabled switches. The complete original-source deltas are `raw/address_parser.patch` and `raw/crf.patch`; source provenance is `raw/source_base_hashes.sha256`.

Built artifacts (SHA-256 in `raw/final_hashes.sha256`) are `artifacts/release/libpostal.a`, `artifacts/release/dump_outputs`, `artifacts/release/bench_ext`, `artifacts/release/fuzz_pipeline`, and `artifacts/original_dump_outputs`. The latter links the preserved `000-baseline/artifacts/libpostal.a` using the same dumper source and O2 link style. Every model-running smoke command uses `research/tools/model_slot.sh`.

## Correctness and thread smoke

The corrected smoke in `raw/smoke_retry/` completed with exit 0. Direct original-versus-release `cmp` checks all exit 0: golden-v1/all (12,863 output lines), novel (12,863), and OSM v3 (11,405). Raw original and release output streams, per-command timestamps, stdout, stderr and exit values are retained beside each check.

`raw/smoke_retry/thread.psM.txt` shows one candidate process entry while parsing golden-v1/all; it is the only observed thread for PID 36919. `raw/thread_static_scan.txt` has no matching thread or parallel primitive in the two changed sources. This is a bounded live observation, not a memory measurement.

The first smoke attempt remains in `raw/smoke.*`. Its parser runs succeeded, but its compare filenames were malformed because the shell helper reused the outer loop variable; its three `cmp` exits are 2. `tools/smoke.sh` was repaired with a local helper variable and rerun into `raw/smoke_retry/`, preserving the failure.

## Decision

The release-mode direct-ID event path is buildable with oracle and statistics instrumentation disabled, and passed the requested paired output and one-thread smoke. No fuzz, memory, or throughput result was collected in this experiment, so none is claimed.
