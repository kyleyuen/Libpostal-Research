# Experiment 176 — benchmark source and candidate contract audit

Date/time/timezone: 2026-10-05 23:20–23:24 PDT (America/Los_Angeles); pinned source snapshot downloaded and validated by 23:24:19 PDT. Per-command start/end timestamps are recorded only where the shell captured them.
Status: **contract audit; no parser source changed, no benchmark timing run**.

## Hypothesis and scope

The user wants the untouched original libpostal source retained and future measurements to compare it with the newest accepted parser architecture. The existing one-command benchmark may not provide a stable way to select a newly accepted build. This audit checks `benchmark/run.sh`, the suite registry and protocol, current handoff, and preserved baseline artifacts. No benchmark or parser code changed; the pinned upstream source was archived under `artifacts/` after the initial audit. No code wrapper was added because the suite already accepts alternate config files and candidate binaries.

The required KnowledgeBase canonical retrieval order was consulted. Its catalog and memory-card index contain no libpostal/CSCI 201 match, so there is no project-specific memory card to apply; the workspace `AGENTS.md`, METHOD, and handoff provide the direct project evidence.

## Findings

- `benchmark/run.sh` rebuilds original libpostal at pinned commit `25099c506612b34b23b1bfe286ca6321fcf06f35`, then builds the candidate from hard-coded `research/final/119-cpi/sources` and `switches.txt`. It therefore preserves reproducibility of that *revision*, but it cannot select an arbitrary newest accepted architecture without editing the script. It also does not copy/archive the full upstream source tree.
- `research/experiments/000-baseline/artifacts/` retains `libpostal.a`, `libpostal.dylib`, `parser_profile`, `parser_profile.c`, an empty `source.patch`, and a git-status snapshot. It did not retain the complete original upstream source. This follow-up archived the tracked source snapshot for commit `25099c506612b34b23b1bfe286ca6321fcf06f35` as a 5.4 MiB upstream source tarball with SHA-256 under this experiment's `artifacts/`; see [source provenance](artifacts/SOURCE_PROVENANCE.md). It was fetched from GitHub's codeload endpoint by full commit hash because the configured `WORK_DIR` source checkout was absent. The tarball is not a Git bundle and carries no refs/history.
- `research/suite/suite.json` supports multiple candidate variants. `run_suite.py --config FILE --variants NAME` can run the original baseline plus a selected candidate using arbitrary harness paths. The checked-in suite's baseline harnesses are under experiments 015/017 and `research/tools/fuzz_baseline`, not under `000-baseline/artifacts`; the checked-in candidate is cpi, and rt8 is disabled. The registry therefore does not directly satisfy METHOD's instruction to use the preserved 000 baseline library for candidate comparisons.
- The current handoff (`research/HANDOFF_2026-10-06.md`) says cpi remains the deliverable; `161fx` is a maintenance candidate pending promotion and P4 validation. It is not yet the newest accepted candidate. The safe candidate selection today remains cpi until acceptance is documented.
- Suite correctness checks compare candidate dumps to baseline and frozen references where configured; timing runs serially; memory and live thread count are separate stages. Full `benchmark/run.sh` runs all stages and rebuilds cpi. Its `--quick` mode is a rough smoke benchmark, not a complete acceptance measurement.

## Minimal contract changes recommended

1. Archive the pinned upstream source tree (or an immutable source archive with commit and SHA-256) in a new versioned baseline artifact location; do not alter `000-baseline` artifacts or the original checksum manifest.
2. Build the suite's original `bench_ext`, `dump_outputs`, and `fuzz_pipeline` harnesses against `000-baseline/artifacts/libpostal.a`, preserving exact compile commands, source headers/commit, and binary hashes in the new archive. Point a generated per-run suite config at those binaries.
3. Keep `benchmark/run.sh`'s default unchanged. Add an explicit candidate selector or use a per-run JSON config generated from `suite.json`; selected candidate must name the accepted source/artifact and all three harnesses. Save the generated config with the suite run so its identity is auditable.
4. Do not add 161fx to the accepted default until it is promoted after the handoff's stated validation. Preserve METHOD gates: byte identity against original on golden-v1 and other registered corpora, single parsing thread, memory no greater than original, and separate corpus timing.

No wrapper was implemented in this record: `--config` already provides the extension point, and choosing a candidate cannot be safely automated until acceptance has a stable artifact path and the original harnesses are rebuilt from the explicitly preserved 000 library. The original source snapshot is archived; baseline harness provenance and candidate selection remain open for the parent task to address.

## Reproduction and checks

Workspace HEAD at inspection: `181737d30977adc9d287e3f7a1cfafd7277525d4`. Workspace had pre-existing uncommitted files; see the captured command output in `raw/`. No compiler or model was run.

| Check | Command | Result |
|---|---|---|
| Runner shell syntax | `bash -n benchmark/run.sh` | exit 0 |
| Suite selection interface | `python3 research/suite/run_suite.py --help` | exit 0; confirms `--config` and `--variants` |
| Frozen profiling integrity | from `profiling/`: `shasum -a 256 -c SHA256SUMS` | exit 0; all 70 listed files OK |
| Initial integrity invocation (failed) | from workspace root: `shasum -a 256 -c profiling/SHA256SUMS` | nonzero: manifest paths are relative to `profiling/`; no files were changed. Corrected invocation above passed. |

No timing, correctness, memory, or thread results are claimed by this audit.

## Source and artifact hashes

Captured 2026-10-05 23:20 PDT:

| Artifact | SHA-256 |
|---|---|
| `benchmark/run.sh` | `b99f658d25f0d7baa356ff3d50830aa09ab575682a208f4fe2270f538edcc34f` |
| `research/suite/suite.json` | `54a066ead53825c3b9a48f6ee535fbad66c740df8de8d8aac5d07d7f30f1da67` |
| `research/suite/run_suite.py` | `ac07848b1adcb7d403998a3f4615d144ed86cc88e9086d320e0c01ce96e86652` |
| `000-baseline/artifacts/libpostal.a` | `30de6ef223346735ac3035654ff0d435e5e621dccc94dc77869807dbf146fe22` |
| `000-baseline/artifacts/libpostal.dylib` | `734ab40beee3c02a64afd35ae468fca6df7deafdcce354da0814343625e557c1` |
| `000-baseline/artifacts/parser_profile` | `0c85b75a71f8af8043a407c366065f1d34ce6d778d54fb920af96f6e8815ce96` |
| `176-benchmark-contract/artifacts/libpostal-25099c506612b34b23b1bfe286ca6321fcf06f35.tar.gz` | `7586542af4db343e744f6128a997dd75bf25b3ddc54ed1af3810ea60d2880c3b` |

## Decision

The benchmark suite is reusable for a newly accepted candidate, but the one-command runner and checked-in suite config are not yet a complete implementation of the requested contract. The original tracked source snapshot is now retained. Remaining gaps are baseline harness provenance from the 000 preserved library and selecting an accepted candidate without hard-coding cpi. No speed claim or candidate decision changed.
