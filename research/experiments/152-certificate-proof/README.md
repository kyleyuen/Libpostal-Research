# Experiment 152 — PLAN A1: certificate proof and fuzz at scale (correctness only, no library change)

Date/time/timezone: created 2026-10-05 (`raw/created.txt`), America/Los_Angeles. Runs 15:15:07 → 15:17:49 (`raw/runs/run.log`).
Status: **A1 passed**.

## Hypothesis, change and kill rule (from `151/PLAN.md`, declared before running)

Hypothesis: the runtime certificate written in [`PROOF.md`](PROOF.md) is sound, and it certifies almost every address.
- **Sound** means that whenever it certifies, the runtime path equals the original's labels.
- **R64** is double precision with rows summed in reverse emission order. It stands for any reassociation, such as merged rows.
- **R32** is float32 weights and sums, with a float32 Viterbi.

Kill rule: any of the following stops the track:
- a certified path that differs from the original (a "violation");
- fallback above 0.1% on a real corpus, or above 1% on fuzz;
- a proof assumption that can't be checked at runtime.

Change: an instrumented copy of the ORIGINAL library (commit 25099c50, original Makefile flags, `-O2`).
- `src/crf.c` is 142's `crf.c` plus an `EXP_CERT` block: `src/cert_block.h`, with 3 call sites in `raw/crf_vs_142.diff`.
- `src/address_parser.c` is identical to 142's.
- The block only reads the original's state. It simulates R64 and R32, applies the certificate, and counts. It never changes scores or labels.

## Reproduction

- Build: `tools/build.sh cert "-DEXP_CERT"` (142's script, retargeted), exit 0. Hashes are in `artifacts/cert/hashes.txt`; source hashes are in `raw/src.sha256`.
- Run: `tools/run.sh`, correctness only and never timed, up to 4 jobs at once. Every set runs on 142's `ctl` original build and on `cert`, with outputs compared by `cmp`.
- Sets: golden-v1, novel, OSM v3, unseen v4–v11, 150's holdout (`audit-input`), DAD-45, and fuzz seeds 2 and 3 with 2,000,000 mutations each over golden-v1 + novel. Reserved v12/v13 were not used.
- Summary: `raw/summary.txt`. The first summary file lost its exit codes to an unquoted path (`raw/summary_attempt1_unquoted.txt`, kept). The script is fixed; the raw logs were unaffected.

## Results (`raw/summary.txt`)

| Set | outputs ctl vs cert | addresses predicted | R64 certified | R32 certified | violations |
|---|---|---:|---:|---:|---:|
| golden-v1 | identical | 12,860 | 100% | 100% | 0 |
| novel | identical | 12,854 | 100% | 12,852 (99.984%) | 0 |
| OSM v3, unseen v4–v11 (9 sets) | identical | 84,825 | 100% each | 100% each | 0 |
| 150 holdout + DAD-45 | identical | 3,298 + 45 | 100% | 100% | 0 |
| fuzz seed 2 (2M) | identical | 1,769,566* | 99.9937% | 99.9440% | 0 |
| fuzz seed 3 (2M) | identical | 1,770,540* | 99.9929% | 99.9418% | 0 |

\* 330 and 353 calls are skipped and excluded from the percentages. A rerun with the counter split (`raw/summary_skipsplit.txt`, build `cert2`, outputs identical again) shows **all of them have T = 0**: no long input went unchecked.

## Interpretation

- **The kill rule is not triggered.** There are 0 violations in ≈3.67M certified checks, and fallback stays within its limits (real ≤ 0.016%, fuzz ≤ 0.059%).
- **The proof's assumptions are checkable at runtime:** finite weights, no FMA contraction (the score recurrences contain no multiplication), and IEEE round-to-nearest.
- **PROOF.md §6 limits where the certificate is needed.** A runtime that adds rows in the original feature-emission order is bit-exact with no certificate, even with records, SIMD across labels or a vectorized Viterbi. The certificate is required only for float32 storage or rows merged ahead of time.

Limitations:
- The certificate is evaluated by simulation inside the original build, not inside a fast runtime.
- R64 covers reverse order only, as one representative reordering. The proof covers every order.
- Fuzz rates come from 2 seeds.

## Decision

A1 passed. Ledger and activity log updated.
