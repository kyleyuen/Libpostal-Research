# Experiment 025 — source audit, API repair, and ASCII borrowing

Date: 2026-10-01 PDT. Starting source commit: `25099c506612b34b23b1bfe286ca6321fcf06f35` (`raw/start.txt`). The original `libpostal/` tracked source has no local tracked changes. The accepted 021 `l4` source was copied to `sources/`; this experiment did not alter 021 or frozen data.

## Hypotheses and variants

1. The packed-weight 021 build may fail on its less-used scoring/API paths and on parser teardown/re-setup. Repair these paths in `fixes` without changing normal parse output or performance.
2. Plain ASCII addresses containing no uppercase letters can avoid an unnecessary normalization allocation and copy. Add this to `borrow` on top of `fixes`; accept only a reproducible gain across the full and novel corpora.

`control` rebuilds the exact packaged 021 sources. `fixes` adds `EXP_AUDIT_FIXES`; `borrow` adds both `EXP_AUDIT_FIXES` and `EXP_ASCII_BORROW`. The conditional changes are in `sources/address_parser.c` and `sources/crf.c`; `source.patch` is the full patch relative to 021. `raw/base_source_hashes.txt`, `raw/binary_hashes.txt`, `raw/final_hashes.txt` and `artifacts/*/hashes.txt` identify sources and binaries. The preserved original comparison binary/library remains under `../000-baseline/artifacts/`.

## Reproduction and identities

- All invoked commands, timestamps, exit codes and stdout/stderr are in `raw/commands/NAME.{meta,stdout,stderr}`. `tools/capture.sh` is the recorder. The build commands are `build_{control,fixes,borrow}.meta`, with exact flags in those files and `tools/build_cc.sh`. Apple clang 17, `-O2 -flto` library, `-O1` tokenizer, whole-library PGO trained only on golden-v1/development, and statically linked mimalloc match 021. The builder uses the preserved 021 scanner object and model path; see script. Build exits: all 0.
- `tools/make_patch.py` and `tools/make_hashes.py` reproduce the patch and final SHA-256 manifest; their exact invocations are captured. `verify_025_hashes_r1`, `verify_frozen_checksums`, and `verify_final_checksum` all exit 0. The first local hash manifest was created before the two reproducibility scripts were added; `make_hashes` replaces it, and the earlier check is retained.
- `raw/input_hashes.txt` identifies all input files and the generated 10,308-address development TSV; `tools/make_dev_tsv.py` records its derivation. `profiling/manifest.json` gives model identities. `verify_model_hashes` checked all nine manifest-listed files against their recorded SHA-256 values, exit 0.
- `tools/verify.sh` is the output gate. The initial `verify_all` run used a direct `cmp` between address-keyed dumper output and ID-keyed frozen JSONL; that comparison failed due to format, retained in `control_cmp_v1`. The corrected `verify_all_r1` run used `compare_frozen.py`, exited 0, and created `raw/VERIFY_DONE`. A one-label corruption was detected by the negative control (`check_negative_control`, expected exit 1).

## Correctness and API findings

- The packaged 021 `l4` aborts with exit 134 when `libpostal_parser_print_features(true)` is used (`api_probe_features`). Its `EXP_ROW_PACK` fallback unconditionally calls `abort()` outside the direct scoring path. `fixes` supports the packed rows in the batched and feature-print paths. The repaired probe exits 0; its output is byte-identical to the preserved original (`cmp_api_features`, exit 0). The same repair passes a full golden-v1 dump with feature dictionaries disabled (`fixes_dict_disable` and `cmp_dict_disable`, exits 0).
- `fixes` clears CRF global capacities, integer-key tables and role/constant IDs on destroy, and frees per-address reusable scratch on parser teardown. The 021 source retains those globals across setup cycles; a stale lean-capacity value can suppress allocation after reload. This is a **source-level risk**, not a reproduced crash. Lifecycle probe output from `fixes` matches the preserved original (`cmp_api_lifecycle`, exit 0).
- The loaded optimized model has freed trie and CSR index arrays, so the original `crf_save` serializer would dereference missing data. `fixes` returns false before writing an invalid file. No save success is claimed for optimized models.
- All three rebuilt variants are byte-identical to the preserved original on golden-v1 (12,863), novel (12,863), and OSM (11,405); golden-v1 and novel also match frozen ID-keyed references. `borrow` passes two 2-million-input pipeline fuzz comparisons. The unchanged control and repaired build are included in the same output checks. Details and failure history: `raw/commands/`.
- `ps -M` counted one row for the running `fixes` parser at six samples, including active parsing (`thread_check`, exit 0). The changed source has no thread creation or offload. The 021 one-thread result remains separately recorded.

## Measurements

All timing sessions passed the AC, Low Power Mode and thermal gate before every run. Development: `run_dev` 5 alternating pairs × 120 passes of 10,308 addresses. Full session: `final_timing` 5 interleaved trials × 19 passes on golden-v1/all and novel, using `../015-integer-keys/artifacts/bench_ext_baseline` linked against the preserved original library, `fixes`, and `borrow`. Long confirmation: `confirm_timing` 3 alternating pairs × 120 passes on each full corpus. All exited 0. Cold pass, CPU time, context switches, setup, and peak footprint were retained per run.

| Session and corpus | `fixes` warm median | `borrow` warm median | Paired median speedup of `borrow` | Decision signal |
|---|---:|---:|---:|---|
| Development, 5 × 120 | 8.123 µs | 7.979 µs | 1.0227×, range 0.9801–1.0534× | Unstable |
| Full, golden-v1, 5 × 19 | 8.15 µs | 8.03 µs | Not used for selection | P10 7.77 vs 7.78 µs; cold 8.53 vs 8.94 µs |
| Full, novel, 5 × 19 | 7.93 µs | 7.93 µs | Not used for selection | P10 7.79 vs 7.83 µs |
| Long confirmation, golden-v1, 3 × 120 | 7.988 µs | 8.045 µs | 0.9991×, range 0.9760–1.0122× | No reproducible gain |
| Long confirmation, novel, 3 × 120 | 8.105 µs | 8.306 µs | 0.9909×, range 0.9573–1.0007× | Slower |

Full same-session preserved-original medians were 35.37 µs (golden-v1) and 35.74 µs (novel). `fixes` yields 4.341× and 4.508× there; **these are session estimates, not a new best result**. In the prior controlled 021 session, the accepted `l4` result was 4.330×/4.358×. The full-session top CPU sampler files were empty, and almost all passes were preempted; use the longer paired results to judge a small increment. `analyze_dev.stdout`, `analyze_final_r1.stdout`, and `analyze_confirm.stdout` contain raw-stat summaries. The initial `analyze_final` path parser emitted an empty table; `analyze_final_r1` corrected it and retained the failed analysis.

Peak footprint in the full session, same `/usr/bin/time -l` mechanism: preserved original max 2,613,774,544 bytes; `fixes` max 2,470,693,168; `borrow` max 2,470,693,192. `raw/commands/analyze_memory.stdout` holds every trial. No memory increase versus the original. The `fixes` and `borrow` figures are effectively equal. This session did not remeasure post-load steady footprint; 021's prior steady comparison remains 2,250 MB vs original 2,492 MB.

The direct maintenance check (`maintenance_timing`, 17:10:04–17:15:55 PDT, exit 0) alternated unchanged `control` and `fixes` for 3 trials × 120 passes on each full set. On golden-v1, pooled warm medians were 8.155 vs 8.023 µs and paired median speedup for `fixes` was 1.0014× (range 0.9163–1.0512×). On novel, 7.953 vs 7.989 µs and paired median 0.9946× (range 0.9899–1.0004×). P10 was 7.869 vs 7.762 µs on golden-v1 and 7.785 vs 7.808 µs on novel. This does **not** establish either a gain or a material regression; noise is substantial, especially in the first golden-v1 pair. Peak maxima were equal within 16,408 bytes. `analyze_maintenance.stdout` retains the exact summaries.

## Decision

**Reject `borrow` as a speed optimization.** Its full and long-run novel measurements do not show a gain, and its cold behavior is mixed. Preserve its source and all unsuccessful results.

**Keep `fixes` as a separate correctness maintenance candidate** while the unchanged 021 `l4` remains the measured final speed deliverable. Its normal parse speed is within noise of a same-session unchanged rebuild. The packed fallback abort is confirmed and repaired; source-level teardown/save guards have output and lifecycle checks but no induced-OOM or save round-trip proof. The direct control-versus-fixes check is in `raw/maintenance_timing/` and `raw/commands/maintenance_*`.
