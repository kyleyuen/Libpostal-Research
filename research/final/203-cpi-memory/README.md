# CPI streaming postcode-graph candidate

This is the promoted, tested candidate for the default parser: it streams the postcode-graph index into the existing compressed representation, avoiding the old index array and compressed output being resident together during setup. It keeps the accepted CPI parser behavior and reports `PC_COMPRESS_STREAM active=1` when the stream path is active.

## Contents

- `sources/`: promoted source and headers.
- `artifacts/cpi-stream-203-v2/`: exact tested `bench_ext`, `dump_outputs`, `fuzz_pipeline`, static library and build-time source/binary hashes.
- `BUILD.md`, `BUILD_SWITCHES.txt`, `MODEL_FILES.sha256`, `build_cc.sh`, `SHA256SUMS`: build identity and promoted-file checksums.

The 203 experiment record and raw validations are in [`research/experiments/203-cpi-memory-regression/README.md`](../../experiments/203-cpi-memory-regression/README.md) and its `FINAL_VALIDATION_REPORT.md`.

## Verified scope

The candidate passed byte-for-byte serialized-output comparison on 14 corpora (130,341 rows), with a negative control that detected a byte mutation. The 14 candidate outputs were compared with cached original outputs. The v14 cached original came from the fresh original-baseline run recorded by experiment 202; the 203 v14 run itself did not generate a new original dump. Two 2-million-input digest/status fuzz checks matched preserved original references; original fuzz was not rerun.

All 84 timed comparisons completed with exit 0. Each warm parsing duration exceeded 10 seconds. The 42 candidate captures all reported the active stream marker. The candidate's measured peak RSS and peak physical footprint were below the paired original in all 42 comparisons. Three paired steady samples (golden-v1, OSM v3, v14) were lower. Six thread-liveness captures (three corpora × two binaries) recorded six live samples apiece; every sample showed one OS thread. Each thread process was intentionally killed after its six samples, separate from exit-0 timing captures.

These are measurements from one Mac with load readings retained in the run logs and environment snapshots. Setup took longer for the candidate, and the speed ratios are warm parse measurements; they do not describe startup improvement. The supported acceptance scope is the tested default parser and these recorded datasets. The existing exported save-after-load limitation under `EXP_PC_COMPRESS` is inherited from accepted 119 and is unchanged by this work.
