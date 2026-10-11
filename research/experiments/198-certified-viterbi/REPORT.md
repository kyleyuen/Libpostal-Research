# Experiment 198 — certified lower precision Viterbi path

Date/time/timezone: started 2026-10-07 18:55 PDT; v3 release and helper checks completed 2026-10-07; America/Los_Angeles.  
Status: development and golden-v1 diagnostic screens passed exact-output and single-thread checks; all13, uninstrumented speed, and matched-memory screens remain pending.  
Evidence type: runtime algorithm implementation, model-free edge checks, successful build, and corrected development/golden-v1 diagnostic screens; performance pending.  
Protocol: `research/METHOD.md`; exact ordered outputs, single-thread parsing and original memory ceilings remain hard gates.

## Hypothesis and change

A lower precision Viterbi recurrence may reduce decoder work while preserving the exact winning ordered labels if every chosen predecessor and final label is separated from alternatives by a rigorous error bound. The implementation uses the existing exact f64 state and state-transition score matrices. It does not use f32 model-score accumulation. It computes a float path, certifies each selected choice, and invokes the original f64 decoder whenever certification or runtime preconditions fail.

V3 source and patch are `sources-v3/` and `raw/source-v3.patch`; manifest SHA-256 `0f3de4e94d1c367930ea83361473b199cc016d2583807c92d0ddacf91c500304`. Base is experiment 119 source, commit `181737d30977adc9d287e3f7a1cfafd7277525d4`; prior feasibility/proof analogs are 151/152. V1 lacked intermediate-magnitude rounding growth; v2 added a conservative multiplier but assumed supported FP mode. Both earlier versions and handoffs are preserved; only v3 is active for screening.

## Operation-based research diagnostics

The candidate adds a float recurrence and an O(T·L²) certificate-bound scan on eligible inputs, in addition to O(T·L) path comparisons. The source computes `eP` and `mst` by scanning all L² state-transition score cells for each of T−1 steps, then scans L state scores and L prior alpha scores per step; it scans L² transition weights once for `trmax`. The diagnostic `viterbi_cert_bound_comparisons` counts only L alternatives in the selected predecessor column per step plus L final-label alternatives (T·L total), not those bound-magnitude scans. It reuses existing address-sized alpha/backpointer workspaces and allocates no additional model table or heap scratch. Any inconclusive margin or unsupported condition reruns exact f64 Viterbi. The corrected diagnostic derivatives measured 10,305/10,305 eligible attempts accepted on development and 12,860/12,860 on golden-v1, with zero exact fallbacks in both. They counted 20,115,200 and 25,118,400 float recurrence predecessor cells respectively, and the same number of transition-score cells in the bound-magnitude passes; selected predecessor/final-label certificate comparisons were 1,211,860 and 1,513,120. These counters confirm the bound scans are quadratic in label count per timestep in addition to the float recurrence. A fully accepted finite corpus establishes coverage on those inputs, not reduced algorithmic work or universal proof.

The bound carries a conservative error value from experiment 152. Each recurrence has three float32 additions, bounded by a `3(u32+u64)` allowance with 1.001 cushion. `M` sums seven nonnegative magnitudes with at most six binary64 additions. The multiplier `1 + 4(u32+u64)` is exactly representable as `1 + 2^-22`, dominating the six binary64 unit-roundoff losses. Overflow or non-finite values fail strict certification. This source argument, not finite corpus separation, is the required basis for exact fallback safety; it does not establish a speedup because the bound scans themselves are quadratic in L per timestep.

## Reproduction

Apple clang build: `-O2 -ffp-contract=off -fno-fast-math`; scanner `-O0`; no LTO/PGO; experiment 119 switches plus `-DEXP_CERT_F32_VITERBI`. Build exit 0; exact command/timestamps/stdout/stderr/exit are `raw/build-v3.*`. Build/model operations and tests did not invoke parser or load model data.

Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`. Source manifest SHA-256: `0f3de4e94d1c367930ea83361473b199cc016d2583807c92d0ddacf91c500304`. Release library SHA-256: `5a0c5af407eafb35caef7f3a3809b9cd2beedbe8c177a8b4f4b6b17813ad16ba`. Handoff: `raw/handoff.json`.

## Model-free helper checks

`tools/model_free_helper_test.py` extracts the exact source bodies of the original f64 decoder and certified decoder into `raw/helper-test.c`, with minimal context/matrix scaffolding. Clear-winner inputs of lengths 1, 4 and 256 certified with exact matching label paths; a 4096-item case rejected and reran the exact fallback. Tie at predecessor and final label, near-tie under f32 resolution, NaN, infinity, below-float-normal and binary64 subnormal operands, conversion/intermediate overflow, unsupported label count, empty sequence, `FE_DOWNWARD`, and AArch64 FPCR FZ/FZ16 rejected. Input score matrices remained unchanged, and rejected paths were checked against exact decoder output. Build and helper test exit 0; logs/harness/source/binary identities are `raw/helper-build.*`, `raw/helper-test.*`, and `raw/helper-test.c`.

Runtime FP guard: AArch64 requires round-to-nearest and FZ=FZ16=0; unsupported architecture or mode uses original f64. Strict `gap > 2δ` at each choice and `>2D` for final path causes ties to fail closed. Non-finite, subnormal, conversion/intermediate overflow also fallback. No fixed sequence-length cap is present.

Source review by experiment 181 found the only in-repo caller of `crf_tagger_score_viterbi` is parser prediction, which discards its score output. On certified success, that internal `score` out-parameter is approximate rather than the original double score. The experiment makes no exact-numeric promise for other non-parser callers of this non-static function; parser labels are the certified observable. Source proof/runtime fallback remains scoped to the source's FP assumptions.

## All measured trials

The corrected diagnostic screens cover 10,308 development and 12,863 golden-v1 inputs, matched retained original ordered output, and observed one parsing thread. Their instrumented resource samples are not release memory measurements. No uninstrumented parser throughput or speedup was measured; all13 and release-profile memory remain pending experiment 186.

| Trial | Baseline or candidate | Addresses/s | CPU/wall seconds | Peak RSS bytes | Raw log |
|---|---|---:|---|---:|---|
| Development diagnostic v5 | candidate vs retained original | — | instrumented duration only; not comparable | diagnostic resource sample only | `research/analysis/parallel-exploration-2026-10-07/raw/198-development-bound-counters-supplement1/` |
| Golden-v1 diagnostic v5 | candidate vs retained original | — | instrumented duration only; not comparable | diagnostic resource sample only | `research/analysis/parallel-exploration-2026-10-07/raw/198-golden-bound-counters-supplement1/` |

## Correctness and interpretation

The model-free helper covers decoder control-flow boundaries only; it is not a universal proof. The dev/golden diagnostic screens establish exact ordered-output equality and one-thread execution on those inputs only, with 100% certificate acceptance in the screened sets. They do not establish universal output equivalence or exact behavior outside the guarded arithmetic assumptions. The runtime guard fails closed for unsupported FP environment/range, while exact fallback handles rejected paths. All13 validation, release-profile memory, and uninstrumented performance remain pending. Experiment 154's memory rejection of a separately stored f64 fallback table informed this scratch-reusing design.

## Decision

V3 passed development/golden-v1 diagnostic output and thread screens and helper edge checks, but the certificate adds an O(T·L²) bound-magnitude scan on eligible inputs. Corpus acceptance alone therefore does not show reduced decoder work or speed. All13 output validation, original-memory gates, and uninstrumented speed remain pending; no optimization or memory improvement is claimed. Parent coordinator owns global ledger, activity log and professor report.
