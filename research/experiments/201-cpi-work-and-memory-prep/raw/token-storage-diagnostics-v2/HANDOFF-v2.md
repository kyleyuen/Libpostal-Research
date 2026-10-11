# 201 handoff v2: corrected CPI185 archive identity

This is a metadata-only correction. It preserves v1 source and patch artifacts;
no source, archive, or build output was changed. No build or parser was run.

## Verified base archive

The CPI185 diagnostic base archive is:

`/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/185-diagnostic-instrumentation/artifacts/build-20261007-1625-pattern-summary-fix/cpi/diagnostic/libpostal.a`

SHA-256 verified from the file, `185-diagnostic-instrumentation/raw/build-20261007-1625-pattern-summary-fix/provenance/cpi.txt` (diagnostic_artifacts_sha256), and `results/full13-retry3/manifest.json`: `c0700a8b99e0f575fe98d8af913435c1d18e24d7896ef98e7e4f91dcc5684b56`. The archive contains 45 members; the exact `ar -t` output is in `cpi185-diagnostic-archive-members.txt`.

CPI185 build provenance is `provenance/cpi.txt` (SHA-256 `951a5e7236ffbe89cca5dfc7468d50f2b17cd9b373cd2bc1e201ffe9499be097`). It records Apple clang 17.0.0, common CPI variant flags including `-DEXP_REUSE` and `-DEXP_FAST_APPEND`, `-DEXP185_DIAG` for diagnostic mode, library `-O2`, scanner `-O0`, and link `-O2 -g`. The same file’s `source_sha256` section is the input-source inventory; this artifact directory does not contain a separate immutable source-tree copy. The 185 CPI inputs used for the directly replaced files are `address_parser.c` 6a699a28479a29d1f10d534b270d2a518a404e502f05bf607a1e8888708a492d, `address_parser.h` 949d0741728ffc0e71d0fc344e3c4728ea31815e219e02a77dcc89d27a706c06, `string_utils.c` 166b520ba48a214b6cf0cfce15c2161fd878e901ea92da916df02239a5e6a51d, `tokens.c` 1f39a53c540f4983cfe16a6c625d2b367179647f8da9d34aadec7d204362ba71, `tokens.h` fc31dd56039c657a29f64df8498e91e25216c558d2c36337dadf55f403653fdb, and `diag.c` e5d6105d730e5e8f8689188707a3304616dcc81ec8b0d45fc4d3c7f8370ec72a. The saved `ar -t` list has 45 lines including the archive symbol table; it enumerates all CPI185 archive objects.

The erroneous `98275eb29cb6378632fc02f923b90e3c1505f191be696e0452cb19d57b2c69dc` hash belongs to `/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/analysis/parallel-exploration-2026-10-07/derivatives/199/artifacts/diagnostic-v9/libpostal.a`, the 199 candidate diagnostic v9 archive output, as shown by `research/analysis/parallel-exploration-2026-10-07/raw/199-diag-build-v9/build-manifest.json` and verified from that file. The v1 handoff incorrectly copied that candidate-output hash into the `base_archive_sha256_expected` field.

## Reuse of v1 source handoff

The source manifest and patch stay exactly as in v1: manifest SHA-256 `d19c0e2ddf64791ca81cfcd217a03e7665520c5609ac95cad2bc05f88464d089`; patch SHA-256 `2814988360b77c9cfa0d39fe465bec5da30c1b94f08cc8f0fb173378739ce7cf`. Compile/replace `address_parser.c`, `tokens.c`, `string_utils.c`, and `diag.c`, with paired `tokens.h`, `diag.h`, and `original/diag.h`. Both modes use this same frozen source, actual CPI185 base archive above, and identical common flags. The only intentional mode delta is `EXP_TOKEN_SLICES` omitted in the ablation control and defined in the 199 candidate.
