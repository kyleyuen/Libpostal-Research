# Experiment 157 — feature-function control-flow cost Y in cpi (deletion bound; diagnostic, NOT exact, never a candidate)

Status: **pre-registered before building** (`raw/created.txt`). Results are appended below.

## Why

155 measured the record design's block after context fill at B_new = 2.111 µs (golden-v1) and 1.509 µs (OSM) cold. That figure excludes the
feature function's own work: control flow, phrase-string joins, vocabulary-frequency probes and integer-id pushes. Those costs erased 149's
gain. **Y** measures them in cpi itself.

## Change

- **Sources.** A copy of `final/119-cpi/sources` where the four string emitters in `features.c` return at entry under `EXP_NO_EMIT` (`raw/features.diff`): `feature_array_add_named`, `_nb`, `_nbl` and the generic `feature_array_add`. No string key is built or looked up.
- **What still runs.** Everything else the feature function does, including `crf_ik_push` integer ids and their placeholders, which remain consistent.
- **Build.** cpi's switches + `EXP_STAGE_STATS` (stage timers) + `EXP_NO_EMIT`, using 119's `build_cc.sh` recipe (PGO, LTO, mimalloc), as 145 did. Output differs by design.

## Pre-registered reading

- **Run.** Variant `nem` against 119 `css` (cpi + stage timers), same split harness, alternating, golden-v1 all and OSM, 4 rounds, gated.
- **Y** = nem's *features*-stage cold µs. It is an upper bound on the hybrid's remaining feature work, because records would also remove the vocabulary probes that Y still contains.
- **Conversion.** Convert Y to the `bench_ext` scale with 155's s. Use the median s of the same corpus in 155 (cross-session, labelled).
- **Hybrid block estimate:** B_hybrid = s·Y + B_new(155).
- **Gate.**
  - If **B_hybrid ≤ 0.95 × cpi's B** (155: 3.155 / 2.308), porting records into cpi has data-backed gain potential, so proceed.
  - If not, porting into cpi is not supported by data, so stop that port and report.

## Results (appended after the run)

**Build.** Attempt 1 failed on a missing `artifacts/scanner_cache` directory (`raw/attempt1_scanner_cache/`, kept; same failure as 144). Attempt 2 exited 0. `bench_split` was linked like 145. Hashes are in `artifacts/nem/hashes.txt`.

**Run.** `tools/run.sh`: 16/16 exit 0, 0 gate failures (`raw/split/run.log`).

Split-harness medians of 4 rounds, cold µs/address:

| | total | context fill | predict (B) | features | lookups | accumulate | Viterbi+rest |
|---|---:|---:|---:|---:|---:|---:|---:|
| golden-v1 css (cpi) | 5.675 | 1.770 | 3.410 | 1.417 | 0.682 | 0.833 | 0.478 |
| golden-v1 nem | 4.205 | 1.626 | 2.091 | **0.923** | 0.182 | 0.506 | 0.483 |
| OSM css (cpi) | 3.768 | 1.006 | 2.341 | 0.886 | 0.421 | 0.616 | 0.417 |
| OSM nem | 2.845 | 0.928 | 1.522 | **0.596** | 0.108 | 0.399 | 0.419 |

**Y** = 0.923 µs (golden-v1) and 0.596 µs (OSM). Building string keys plus looking them up costs ≈1.0 µs (golden-v1) of cpi's 2.1 µs features + lookups. The rest is control flow, vocabulary probes, phrase joins and integer ids.

**Gate** (s from 155 medians: 0.925 golden-v1 / 0.885 OSM; cross-session, labelled):

| | golden-v1 | OSM |
|---|---:|---:|
| B_hybrid = s·Y + B_new | 0.854 + 2.111 = 2.965 | 0.527 + 1.509 = 2.036 |
| ratio to cpi's B | 2.965 / 3.155 = **0.940** | 2.036 / 2.308 = **0.882** |
| gate (≤ 0.95) | **PASS** (narrow) | **PASS** |

**Whole-parser estimate** (UNVERIFIED; 155's C − B + B_hybrid):
- golden-v1: 5.057 µs, ≈6.29× against 6.06×, i.e. **≈+4%**;
- OSM: 3.586 µs, ≈5.50× against 5.11×, i.e. **≈+8%**.

Two effects pull in opposite directions:
- **Conservative:** Y still includes the vocabulary probes that records remove.
- **Optimistic:** B_new is a replay, with no context-fill cache pressure.

## Decision

The gate passes. Under the user's rule (byte-identical output, memory ≤ original, data-backed gain), porting records into cpi proceeds. **The expected gain is modest (≈4–8%).** The larger lever is the feature function's own work (Y ≈ 0.6–0.9 µs), which records partly absorb by supplying frequencies and neighbour data.
