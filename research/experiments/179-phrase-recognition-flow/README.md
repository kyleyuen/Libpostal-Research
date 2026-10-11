# Experiment 179 — root-byte phrase dispatch over original tries

Date: 2026-10-06 PDT. Status: **diagnostic prototype; no speed benchmark or speed claim**.

## Hypothesis and scope

Before each original trie walk, use a once-per-context-fill 256-bit mask of valid root bytes to skip token starts that cannot match any phrase. The component and postal tries share normalized-admin input, so their masks are reused. Trie storage, tail matching, continuation/restart rules, match ordering, and payloads remain original. This is a narrow path diagnostic / dispatch filter, not a merged traversal scheduler or a rebuilt phrase architecture. Inspection showed that the separate tries have restart/tail-compressed state quirks; merging those state machines would be a larger rewrite and was deliberately avoided.

Candidate is derived from upstream libpostal commit `25099c506612b34b23b1bfe286ca6321fcf06f35`; immutable source tarball and SHA-256 are in experiment 176 (`artifacts/libpostal-25099c506612b34b23b1bfe286ca6321fcf06f35.tar.gz`, SHA-256 `7586542af4db343e744f6128a997dd75bf25b3ddc54ed1af3810ea60d2880c3b`). Scratch source trees and built binaries are local under this experiment. Candidate source hashes and artifact hashes: `raw/final-artifact-hashes.sha256`. Control/candidate executable/library manifest hashes: `artifacts/{control,root-gate}/hashes.sha256`.

## Reproduction and captured evidence

Use `tools/build.sh` for serial O2 builds and `tools/run_logged.sh NAME -- COMMAND ...` for the captured-run format. The exact argv, start/end timestamps, stdout, stderr, and exit status for every logged operation are the corresponding files under `raw/`. Model commands were gated through `research/tools/model_slot.sh`; there were no concurrent model-heavy runs. Build/compiler flags and harness paths are recorded in `tools/build.sh` and `raw/build-with-root-stats.command`.

Successful exactness checks on the latest candidate build:

- Golden-v1 final parser JSON, O1, O2, O3k, O3i, and O4 matched control and frozen references byte-for-byte (`raw/validate-golden-stats.stdout`, exit 0). Final JSON: 2,153,786 bytes, SHA-256 `45574ee2b1972f1acf2dbf53eb1fc2c25f8e1972cd5c22e8071fe3606c5c8c63`.
- Independent OSM v3 final parser JSON and all five oracles matched control/references byte-for-byte (`raw/validate-osm-stats.stdout`, exit 0). Final JSON: 1,709,015 bytes, SHA-256 `e8a48875b12310ef77f41603d0c6e54edc48c25657596407769d8e4deadb051b`.
- Development final output matched control (`raw/development-output-hashes.stdout`, exit 0); 20,000-case fuzz seed 1 matched (`raw/control-fuzz20k` and `raw/root-gate-fuzz20k`, comparison command/status in `raw/`). Each validator's built-in one-byte negative control detected the corruption.
- Latest instrumented diagnostic counts: golden 144,623 eligible start checks / 1,423 skipped starts; OSM 116,631 / 1,401 (`raw/root-gate-{golden,osm}-stats.stderr`). This quantifies limited work avoided, not net work or speed.
- `/usr/bin/time -l` provided noisy one-run memory diagnostics. Latest instrumented candidate max RSS was 2,029,535,232 bytes on golden and 2,489,057,280 bytes on OSM; peak footprint was 2,611,366,096 and 2,489,665,648 bytes respectively. Earlier control/candidate samples vary substantially. These are not a memory qualification or comparative performance result.

## Limits / decision

No timing benchmark was run. Only the stated golden, OSM, development, and bounded fuzz checks are claimed; no full suite or broad randomized matrix. Parsing/build orchestration was serial and the source change adds no worker/thread creation, but no independent live thread-count sampler was captured. The mask costs roughly 64 bytes plus alignment in each parser context and initializes lazily. Counts show only about 1.0% (golden) and 1.2% (OSM) of eligible attempts skipped; avoided trie work may be small relative to mask checks and context storage. Thus this is an exact, bounded structural probe, not an accepted optimization. A real third architecture would need a correctly fused token/phrase traversal and separate cost measurement after exactness qualification.

Initial setup failures are preserved in raw logs: two oracle-hook copy attempts used a wrong path and exited nonzero; a compare attempt referenced the wrong helper path. Corrected copies/builds and all validations succeeded. An initial no-oracle build is also retained under `artifacts/initial-no-oracle/`; final evidence uses the instrumented build.

No shared ledger, activity log, report, benchmark, original/final source, golden data, or experiments 174/175 were modified.
