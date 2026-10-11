# Portable runner setup check, version 1

This record checks that `benchmark/run.sh` can prepare a fresh work directory using the promoted 203 source. It does not run the benchmark suite.

## Execution

- Environment gate: `research/tools/env_gate.sh` returned `GATE OK: Now drawing from 'AC Power'` at the start and before the output smoke.
- Shared serialization: setup and parser commands were invoked through `research/tools/run_locked.py`.
- Setup command: `env WORK_DIR=/tmp/libpostal-speedup-203-setup benchmark/run.sh --model-dir /Users/kyle/.cache/libpostal-profile/libpostal --setup-only`.
- Setup interval: local terminal output began at `18:48:15` and reported successful completion at `19:00:08` on 2026-10-10 (UTC rollover: 2026-10-11). Launcher exited normally after writing `/tmp/libpostal-speedup-203-setup/suite.json`.
- The pinned libpostal source commit was `25099c506612b34b23b1bfe286ca6321fcf06f35`. The model was reused from the existing cache, not downloaded; all seven paths in `benchmark/model_sha256.txt` passed (`logs/model_check.log`).
- Setup performed the original build, candidate profile-generation build, PGO training on `profiling/golden-v1/development.txt`, profile merge, and final candidate build. Captured logs are in `logs/`; fresh binary hashes are in `binaries.sha256`.
- `suite.json` identifies the candidate as `memory-fixed CPI from research/final/203-cpi-memory (streamed postcode graph loader)`.

## Source identity

The promoted package is pinned by `SHA256SUMS` and `SHA256SUMS.sha256`, with a separate `SOURCE_MANIFEST.sha256`. The runner copied the promoted `sources/` and all headers into its fresh build tree. Its source/header hashes match the tested `artifacts/cpi-stream-203-v2/hashes.txt`; notable identities are:

- `address_parser.c`: `ca2e56f6d2f814954ae7d9b6e5189486997fa0a7d0239e14586a3ee21d8ef992`
- `stream_postcode_graph.h`: `ba25c92864cdec353c0529119ced00a780940bdf4569f3dbc74a81201c3c56b6`
- build switches: `BUILD_SWITCHES.txt`; the runner uses the shared accepted 119 switch file plus its existing base switches.

## Output smoke

`smoke-input.txt` contains the first eight development inputs. The original and fresh candidate dumpers each emitted eight JSONL rows with empty stderr. Their output files are byte-identical (`cmp` exit 0; both SHA-256 `813db192107fd01edd04f341094361e3db70adeee939f58031355c040586e37f`). A second candidate invocation with `PC_COMPRESS_VERBOSE=1` produced the same output and logged `PC_COMPRESS_STREAM active=1 rows=5483892 edges=122300179 bytes=151033002`.

The first attempted candidate shell command had a misspelled stderr redirection path; the shell exited before launching the parser. It is preserved in `cpi203-attempt1.txt`. The corrected original/candidate captures and active-stream check are retained alongside it.

## Validation

- `bash -n benchmark/run.sh`: pass.
- `benchmark/run.sh --help`: pass; documents default source 203 and the `--cpi-source 119` historical-source option.
- `/tmp/libpostal-speedup-203-setup/suite.json` was generated, but timing, memory, threads, fuzz, and the full suite were not run by this check.
