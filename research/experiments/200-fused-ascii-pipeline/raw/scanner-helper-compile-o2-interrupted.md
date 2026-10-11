# Interrupted model-free helper compile attempt

Purpose: compile the generated model-free helper with the scanner source to check candidate scanner/token semantics; no parser/model workload was invoked.

Command: `python3 research/tools/run_locked.py -- /usr/bin/clang -std=c11 -O2 -Wall -Wextra -I libpostal/src research/experiments/200-fused-ascii-pipeline/raw/scanner-helper-test.c libpostal/src/scanner.c -o research/experiments/200-fused-ascii-pipeline/raw/scanner-helper-test-v2`

The compiler ran under `research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock`. It was interrupted after approximately three minutes of single-core compilation to release the exclusive workload lane for the benchmark worker. The wrapper returned exit code 130 with `KeyboardInterrupt`. Stdout/stderr were not redirected to files; tool output contained extensive compiler warnings and was truncated, so complete logs are unavailable. The build was not treated as successful. The equivalent helper was subsequently linked to the unchanged experiment-198 scanner object built at `-O0`; the helper executable passed (exit 0). That scanner-object reuse is identified in `raw/handoff-v2.json`.
