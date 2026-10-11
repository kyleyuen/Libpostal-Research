# Audit command log

Host/workspace timezone: America/Los_Angeles. Session inspected around 2026-10-05 23:20–23:22 PDT, workspace HEAD `181737d30977adc9d287e3f7a1cfafd7277525d4`. Exact per-command start/end timestamps were not captured by the shell and are not reconstructed here.

| Timestamp capture | Exact command | Exit | Captured output |
|---|---|---:|---|
| Not captured | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk shasum -a 256 -c profiling/SHA256SUMS` (workspace root) | 1 | `wrong-directory-integrity.log` / `.exit`; manifest references paths relative to `profiling/`; no files changed |
| Not captured | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk shasum -a 256 -c SHA256SUMS` (working directory `profiling/`) | 0 | `golden-integrity.log` / `.exit`; all listed entries OK |
| Not captured | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk python3 research/suite/run_suite.py --help` | 0 | `suite-help.log` / `.exit` |
| Not captured | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk bash -n benchmark/run.sh` | 0 | `bash-syntax.log` / `.exit` |
| Not captured | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk ls -ld "$HOME/.cache/libpostal-speedup/libpostal"` | 1 | `cache-check.log` / `.exit`; configured default checkout path is absent |
| Not captured; completed before 23:24:19 PDT | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk curl -fL --retry 2 --output research/experiments/176-benchmark-contract/artifacts/libpostal-25099c506612b34b23b1bfe286ca6321fcf06f35.tar.gz https://codeload.github.com/openvenues/libpostal/tar.gz/25099c506612b34b23b1bfe286ca6321fcf06f35` | 0 | `source-fetch.stdout`, `source-fetch.stderr`, `source-fetch.exit` |
| Not captured | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk gzip -t research/experiments/176-benchmark-contract/artifacts/libpostal-25099c506612b34b23b1bfe286ca6321fcf06f35.tar.gz` | 0 | `source-gzip-check.exit`; stdout/stderr empty |
| Not captured | `/Users/kyle/Library/Application\ Support/Headroom/headroom/bin/rtk shasum -a 256 -c SHA256SUMS` (working directory `research/experiments/176-benchmark-contract/artifacts/`) | 0 | `source-sha-check.log` / `.exit`; archive verified |

Read-only inspection commands also used `rtk cat`, `rtk rg --files`, `rtk rg`, `rtk ls`, `rtk shasum`, `rtk date`, `rtk git rev-parse`, and `rtk git status`; see the experiment report for inspected paths and hashes. One `rtk find ... -exec` invocation was unsupported by RTK and did not run; the listing was obtained with `rtk ls` instead. No `run_suite.py` benchmark stage was invoked.
