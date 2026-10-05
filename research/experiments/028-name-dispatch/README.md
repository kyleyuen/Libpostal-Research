# Experiment 028 — model-derived feature-name dispatch

Date/time/timezone: 2026-10-01T18:34:46-07:00, America/Los_Angeles. Created by copying TEMPLATE.md before any source edit.
Status: source-only feasibility preparation; model inspector has not been compiled or run. Work stopped before Mac suspension.

## Hypothesis and change

Measured bottleneck and why this change should help: The 026 sample of unchanged 021 attributed 10.9% of parser samples exclusively to crf_dict_lookup_array. The current dictionary splits every remaining string feature, hashes its feature name, probes a 256-entry name table, and compares the name. A model-derived direct dispatch by the first name byte and name length might skip the hash/probe when that bucket has only one name. This is a hypothesis, not a speed claim. The 012 inline hash/compare lost under plain -O2, then 019's PGO/LTO retry won and is already present in 021; this experiment does not repeat it.
Exact implementation change: pending model-name inventory and bucket-collision count. If coverage justifies a candidate, add a small final-dictionary table from model names only, compare the complete name before accepting a unique bucket, and fall back to the current lookup on ambiguity or unsupported lengths. No benchmark-address-derived choices.
Base commit and candidate commit (or saved binary patch, including untracked source): pinned libpostal checkout 25099c506612b34b23b1bfe286ca6321fcf06f35; unchanged 021 l4 sources at research/final/021-l4/sources. Candidate pending.
Incremental comparison base, if different from original libpostal: unchanged 021 l4; final acceptance would still require preserved-original comparison.

## Reproduction

Hardware/OS/compiler: Apple M4 Pro, macOS 26.5.2, Apple clang 17. No build yet.
Corpus/split/order and SHA-256: no input-corpus run yet. Any model-name audit will read only the model, not development or golden addresses.
Model hashes: pinned profiling/manifest.json; exact active hashes will be captured with the inventory.
Build flags, exact commands and command-log path: pending; logs under raw/commands/.
Harness and library hashes: pending.
Warm-up, duration, trial order, other workload/thermal observations: pending; CPU-heavy scans, builds and timing held during 026 Linux diagnostic work.

## All measured trials

| Trial | Baseline or candidate | Addresses/s | CPU/wall seconds | Peak RSS bytes | Raw log |
|---|---|---:|---|---:|---|
| — | — | — | — | — | — |

Median throughput, spread and speedup:
Peak RSS difference (bytes and percent):
Steady-state memory evidence or limitation:
Any excluded trials, original logs and reasons:

## Correctness and interpretation

Exact output comparison count, mismatches and log: pending; hard gate requires all golden-v1 outputs byte-identical and one parsing thread.
Additional tests for the changed code: pending; first count all model names and uniqueness by dispatch bucket. If a candidate is built, differential-test arbitrary names including same-bucket misses and collision fallback.
Profile evidence explaining the change (if collected): research/experiments/026-cross-platform/profile/README.md; 10.9% is the entire CRF lookup routine, not measured name-lookup cost.
Development result vs validation/final result: none yet.
Known limitations and remaining uncertainty: Bucket coverage and gain are unknown. The final CRF dictionary is produced by fd_int_split, so a fast table must initialize after that split and remain correct through construction/failure fallback. Any table is model-derived and must not memoize results across addresses.

## Decision

Accepted / rejected / inconclusive, with evidence: pending feasibility count.
Next action: after the Mac resumes and the shared CPU window is free, capture a build of tools/build_model_names.sh, then capture artifacts/model_names with no address corpus. Inspect the exact hex name inventory, collision groups and fast64 coverage in its output. If coverage is promising, implement the candidate in this experiment's copied sources and run differential/exact-output gates before timing. Hold all model setup, builds and timing until that window.
Ledger, report and activity log updated: parent integration pending.

## Suspension handoff

The 021 sources were copied unchanged into sources/ after this TEMPLATE record was created; address_parser.h is an identical upstream copy needed by the inspector. [source_identity](raw/commands/source_identity.stdout) reports only that extra header, and [header_identity](raw/commands/header_identity.meta) confirms its bytes. [model_names.c](tools/model_names.c) only reads the final loaded state/trans dictionary names after libpostal setup and reports each exact name in hexadecimal, its (first byte, length) peer count, and fast64 coverage. [build_model_names.sh](tools/build_model_names.sh) links it to the preserved 021 l4 library. Neither the inspector nor the parser has been built or run for 028; no model enumeration, candidate patch, speed or memory result exists. The build script passed shell syntax and [pre-source hashes](raw/commands/pre_source_hashes.stdout) are saved. Do not infer bucket coverage from 006's historical count of 29 state and 2 transition names; inspect the current model when CPU work resumes.

Never replace missing measurements with zero or projected gains. Retain failed
attempts and do not modify frozen references to make the candidate pass.
