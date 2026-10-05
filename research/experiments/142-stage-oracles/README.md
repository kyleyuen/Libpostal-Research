# Experiment 142 — stage oracles from the original library (REWRITE_PLAN M0)

Date/time/timezone: created 2026-10-04T19:23:04-0700 (`raw/created.txt`), America/Los_Angeles. Commands: `raw/commands.txt`.
Status: complete — M0 passed

## Hypothesis and change

Approved by the user 2026-10-04 ("now run the M0 + M1"). Plan: `research/plans/REWRITE_PLAN_2026-10-04.md` §3, milestone M0.

Hypothesis (written before building):
- Adding `fprintf`-only hooks (`EXP_ORACLE`) to the ORIGINAL `address_parser.c` and `crf.c` changes no computation.
- An `-O2` build of the original sources therefore reproduces the original outputs exactly, with dumps off and with dumps on (golden-v1 sha `45574ee2…`, novel `806b8a81…`, OSM and fuzz-seed-1 prefix identical).
- The dumps are deterministic (two runs are byte-identical), and the comparator fails at the right record on perturbed copies.

Kill rule (from the plan): if the identity control fails, no rewrite work starts until it is fixed.

Change: oracle hooks only, guarded by `EXP_ORACLE`, active at run time only when `ORACLE_DIR` is set. Base: libpostal commit `25099c50` (no tracked changes, `raw/base_commit.txt`); base source hashes in `raw/base_sources.sha256`.
No other source file is modified.

Oracle files (format in plan §3.3), all keyed by `a` = 0-based index of the `address_parser_parse` call and `t` = token index:
- `O1.txt` tokens after separator/ignorable filtering;
- `O2.txt` context after `address_parser_context_fill`;
- `O3k.txt` / `O3i.txt` per-token feature keys and resolved ids, in emission order with duplicates kept;
- `O4.txt` per-token state row (dense) and state-transition row.

Declared deviation from plan §3.3, fixed before running: the `L·L` transition row is written sparse, as `index:hex64` for every entry whose bit pattern is not `+0.0`. This avoids ≈0.7 GB per corpus. It is still bit-exact, because `-0.0` has non-zero bits and is therefore recorded.

Addresses that take the whole-string single-phrase shortcut have no O3/O4 records, because the original skips prediction for them.

## Reproduction

- Patch: `raw/address_parser.diff`, `raw/crf.diff`. Additions only: no original line is removed or changed; every addition is inside `#ifdef EXP_ORACLE`. Patched-source hashes are in `raw/patched_sources.sha256`.
- Build: `tools/build.sh ctl ""` and `tools/build.sh orc "-DEXP_ORACLE"` use the original Makefile flags (`-O2`, scanner `-O0`, system malloc, no PGO/LTO, Apple clang 17). Exits 0 (`raw/build_*`); the warning count is identical with and without the hooks. Artifact hashes are in `artifacts/{ctl,orc}/hashes.txt`. Harnesses: 003 `dump_outputs.c`, `research/tools/fuzz_pipeline.c` (unchanged).
- Runs:
  - `tools/run_identity.sh` (19:24:57–19:26:20) and `tools/run_dumps.sh` (19:26:45–19:27:19); `raw/id/run.log`, all exits 0;
  - `tools/negctl.sh` (`raw/negctl/`);
  - gzip and hashes (`research/oracles/v1/SHA256SUMS`, `SHA256SUMS.uncompressed`).
  Correctness runs only, never timed; up to 4 ran concurrently.
- Model: as in 141 `raw/model_hashes.txt`.

## Results

Identity control (`raw/identity_summary.txt`, `raw/identity_summary2.txt`):

| Check | Result |
|---|---|
| ctl golden-v1 / novel sha | `45574ee2…` / `806b8a81…` ✔ |
| ctl OSM vs 017 baseline; ctl fuzz s1 (2,000,000) vs `base_s1.txt` | identical ✔ / identical ✔ |
| orc, dumps **off**, golden-v1 sha | `45574ee2…` ✔ |
| orc, dumps **on**: golden-v1 sha (+ frozen `baseline-outputs.jsonl`: IDENTICAL), novel sha, OSM, fuzz s1 first 200,000 | all identical ✔ |
| determinism: second golden-v1 dump, all five oracles | byte-identical ✔ |
| negative controls (O3i id +1, O4 one bit, O1 offset +1 at a=5000) | comparator exit 1 and `cmp` exit 1 for each; first difference reported at a=5000 and the right token ✔ |
| positive control (O3i vs itself) | exit 0 ✔ |

Oracle sizes (records): golden-v1 has 12,863 addresses, 75,656 predicted tokens and 702,370 feature records (O3k/O3i). Novel has 12,863 / 73,749; OSM 11,405 / 61,157; fuzz 178,955 parsed addresses (+21,045 invalid UTF-8 skipped before parsing) / 1,361,822 tokens. Total 1.5 GB gzipped under `research/oracles/v1/`.

## Decision

**M0 passed.** The kill rule (identity control) is not triggered. The oracles are trusted for H-builds.
Limitation: the oracle covers the CRF path only, because the averaged-perceptron path is unused by this model.
Next action: M1 (experiment 143).
