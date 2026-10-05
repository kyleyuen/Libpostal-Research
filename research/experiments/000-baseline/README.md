# Experiment 000 — original libpostal baseline

Date: September 28, 2026. Status: complete; no optimization applied.

## Results

| Trial | Addresses/s | Mean µs/address | Peak RSS bytes | Raw output |
|---|---:|---:|---:|---|
| 1 | 27,542.83 | 36.31 | 2,613,886,976 | [Trial 1](../../../profiling/golden-v1/baseline-1.txt) |
| 2 | 28,602.47 | 34.96 | 2,613,690,368 | [Trial 2](../../../profiling/golden-v1/baseline-2.txt) |
| 3 | 27,821.28 | 35.94 | 2,613,690,368 | [Trial 3](../../../profiling/golden-v1/baseline-3.txt) |

Median throughput: **27,821.28 addresses/s**. Individual trial means are not
per-address latency percentiles. The baseline has no candidate speedup yet.

## Evidence index

| Evidence | Location |
|---|---|
| Dataset and split definition | [Dataset README](../../../profiling/golden-v1/README.md) |
| Full timing inputs | [all.txt](../../../profiling/golden-v1/all.txt) |
| Exact baseline outputs | [baseline-outputs.jsonl](../../../profiling/golden-v1/baseline-outputs.jsonl) |
| Positive output check | [Check log](../../../profiling/golden-v1/check-outputs.log) |
| Rebuild and negative-control checks | [Validation log](../../../profiling/golden-v1/validation-checks.log) |
| Raw stack samples | [parser.sample.txt](../../../profiling/golden-v1/parser.sample.txt) |
| Aggregated profile | [profile-summary.json](../../../profiling/golden-v1/profile-summary.json) |
| Model/source/compiler hashes | [Manifest](../../../profiling/manifest.json) |
| Original evidence integrity | [SHA256SUMS](../../../profiling/SHA256SUMS) |
| Build and download output | [Setup logs](../../../profiling/setup-logs/) |
| Preserved baseline harness source | [parser_profile.c](artifacts/parser_profile.c) |
| Preserved baseline executable | [parser_profile](artifacts/parser_profile) |
| Preserved static library | [libpostal.a](artifacts/libpostal.a) |
| Preserved dynamic library | [libpostal.dylib](artifacts/libpostal.dylib) |
| Source difference from original commit | [source.patch](artifacts/source.patch) (empty) |
| Generated/untracked build state | [git-status.txt](artifacts/git-status.txt) |

The copied executable and libraries were checked against the original manifest.
Their model directory remains `/Users/kyle/.cache/libpostal-profile/libpostal`.
Keep these files unchanged when building future candidates.

## Rerun preserved baseline

From the workspace root:

```sh
research/experiments/000-baseline/artifacts/parser_profile profiling/golden-v1/all.txt 10
python3 profiling/check_outputs.py check --library "$PWD/research/experiments/000-baseline/artifacts/libpostal.dylib"
```

Save new trials in a new experiment directory; do not overwrite the historical
outputs above. The source build, model downloads and flags are documented in
the [original report](../../../profiling/initial-70/README.md).
