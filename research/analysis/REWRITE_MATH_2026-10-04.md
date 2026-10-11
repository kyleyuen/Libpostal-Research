# How possible is the rewrite? A cost model from measurements (2026-10-04, experiments 141–145)

The user asked (2026-10-04) for a mathematical answer before more building. Every input below is a measured number with its experiment. Every output is a projection, marked UNVERIFIED, until a built candidate is timed. Scripts: `experiments/145-bigram-deletion-bound/tools/model2.py` (→ `raw/model2.stdout`) and `tools/coverage_122.py`.

All times are µs per address on the split (stage-timer) harness, cold. Speedups are projected by applying the split-harness *fraction* f to 121's validated `bench_ext` speedups: new = old / (1 − f).

Deltas from different sessions are combined only as differences measured against cpi *within their own session*: 122 from 01:51–02:22, 145 from 20:37–20:43. No ratio mixes absolute times from two sessions.

## 1. What must be removed for 7.8×

| | golden-v1 dev | OSM |
|---|---:|---:|
| cpi total (145 `css`) | 5.534 | 3.640 |
| validated speedup (121) | 6.08× | 5.78× |
| 7.8× needs | −22.1% = **1.22 µs** | −26.0% = **0.95 µs** |
| fixed by byte-identity (warm accumulate + Viterbi, 141) | ≈0.90 | ≈0.83 |
| front half: context fill + features + lookups (141) | 3.77 | 2.28 |

## 2. Feasibility (M0, M1): the rewrite is possible

- **Exactness is checkable (142).** Original-library oracles exist for every stage. A one-bit change is caught at the right address and token.
- **The ids exist in the model (143).** 99.8–100% of runtime word-family keys split into model units. 0.23–0.46% of model keys are ambiguous, and none splits two ways.
- **Memory most likely falls (143, 122).** The gross tables are 142.6 MB, but they replace existing entries. 122 measured −48 MB peak for the pair move. Confirmation needs a built candidate (declared kill rule: peak > original − 20 MB).

## 3. The bigram card: measured ceiling and realistic bracket (145, 122)

- **Ceiling.** Deleting the bigram string path outright (not exact) saves **0.391 µs dev (7.1%) and 0.330 µs OSM (9.1%)** in features + lookups.
- **What an exact rewrite keeps.** It must still probe a table for every bigram, and cpi already batches and prefetches those probes within the lookup stage.
- **122's measured savings.** 122 moved the bigram keys whose two parts are vocabulary words to an integer table, which covers c = 0.772 of runtime bigrams on dev and 0.819 on OSM (measured from the 142 oracles). Rescaled to full coverage:
  - integer probing saves 0.041 / 0.043 µs over string probing;
  - each canonicalization lookup costs 14.3 / 10.5 ns.

ΔT = c_build_saved + lookup_saving − key_compose − unit_cost

| case | dev | OSM |
|---|---|---|
| pessimistic (one fresh lookup per token for unit ids) | 0.119 µs = 2.1% → 6.21× | 0.138 µs = 3.8% → 6.00× |
| middle (131: 38% of parts already cached) | 2.7% | 4.4% |
| optimistic (unit ids free) | 0.203 µs = 3.7% → 6.31× | 0.194 µs = 5.3% → 6.10× |
| ceiling (string path deleted) | 7.1% → 6.54× | 9.1% → 6.35× |

**The one upside this model omits.** With integer keys, the pair-table slots are computable right after context fill. That is the "prefetch a whole stage ahead, from data in hand" pattern that won in 074/077/114, and cpi's within-stage batch cannot do it.

The ceiling bounds this term. The honest range is therefore *pessimistic … ceiling*, with the realistic bracket inside it.

## 4. The whole integer pipeline (bigrams + the remaining string families)

- **Phrase-type families** (2.32 / 1.80 keys per address): 130 already turned exactly these into ids and measured **+1.1% golden-v1 / −0.2% OSM**. That measured result is used as is.
- **The rest** (word, postcode, neighbour and transition unigrams; 5.62 / 3.82 keys per address) is **EXTRAPOLATED** at the bigram per-key ceiling (40.7 / 39.0 ns) times the bigram card's realised share.

| | dev | OSM |
|---|---|---|
| realistic whole pipeline | 0.25–0.38 µs = **4.5–6.9% → 6.36–6.53×** | 0.19–0.28 µs = **5.3–7.5% → 6.10–6.25×** |
| outer ceiling (every remaining string path free) | 12.3% → 6.93× | 13.2% → 6.65× |
| still needed for 7.8× after the realistic pipeline | 0.84–0.97 µs | 0.67–0.75 µs |
| still needed after the outer ceiling | 0.54 µs | 0.47 µs |

## 5. Is 7.8× reachable?

After a realistic pipeline, OSM still needs 0.67–0.75 µs. That is about 27–30% of the remaining non-fixed time (≈3.33 − 0.83 µs). It is also larger than context fill's entire cold−warm difference on OSM (0.60 µs), which is the largest pool of time not already claimed. This is a size comparison, not a proof of impossibility.

The memory-side experiments listed in handoff §6 (15 rejected) each recovered a few percent at most [corrected 2026-10-05: an earlier draft said 'thirty-plus', which was not counted]. **No identified exact path reaches 7.8×.** Even the outer ceiling of the whole pipeline, with every remaining string path made free, stops at ≈6.9× golden-v1 and ≈6.7× OSM.

## 6. On "speedups from approximation and math"

- **Exact algebraic restructuring** of the score computation is ruled out by byte-identity, because it changes floating-point order (hard rule 1).
- **Approximations** (pruning features, float32 weights, skipping families) change output. 145 is a free data point: deleting the bigram family alone changes 843 of 12,863 golden-v1 parses (6.6%) and buys only 7–9% speed. That is a poor trade, and accuracy would have to be scored against ground truth.
- **Where approximation could pay off:** only at the level of the fixed bucket, i.e. a smaller or retrained model. That is a rule change for the user and the professor to decide, not an optimisation (handoff §7).

## 7. Correction after experiment 147 (2026-10-04T21:39:00-0700)

147 measured cpi's existing integer bigram path (122's `EXP_INT_KEYS_PAIRS`, two canonicalizations + `fdi_key` + `crf_ik_push` per bigram) section by section.
- **Result.** It costs *more* warm compute than the string emission it replaces: features +0.038 / +0.023 / +0.021 µs warm (dev / OSM / u11), mainly in the prev-word section, against lookups −0.011…−0.014.
- **The pessimistic case was wrong.** §3/§4 assumed per-token unit ids would make integer emission cheaper than string emission. For 122-style emission the **lower bound is ≈ 0% (or slightly negative)**, not +2.1% / +3.6%.
- **The upper end is untested.** It requires a design whose per-bigram emission costs less than cpi's string emission, about 10 ns per bigram (145: 0.099 µs warm for 9.6 bigrams).
  - Exact ceiling of the bigram card, even if integer emission were free: 0.171 + 0.041 = 0.21 µs dev (3.8%) and 0.20 µs OSM (5.6%).
  - M3's +1.5% threshold needs ≈0.083 µs dev and ≈0.055 µs OSM.
- **The outer ceilings in §4 are unchanged.**
- **Corrected realistic range for the whole integer pipeline: ≈ 0% to +7.5% (UNVERIFIED), with the bigram card itself 0–5%.**
- **Next.** A line-level warm profile decides whether a cheaper per-token design exists before any R-build (declared rule in experiment 148).
- 147's cold numbers are contaminated: interactive load, swap 9.9 GB used, Spotlight, run within minutes of 146's large parallel verification.

## 8. Outcome: the integer-pipeline track is stopped (2026-10-04T22:17:18-0700, experiments 146–149)

- **M2 passed (146):** the integer bigram design is exact on the original code at every stage.
- **M3's R-build (149) on cpi** (122's integer pairs + a content-validated canonicalization cache + phrase-unit ids) is exact everywhere but **not faster**: golden-v1 0.985–0.993×, OSM 0.996–1.012×, warm features +0.035…+0.064 µs.
- **Why the 145 ceiling cannot be reached this way.** cpi's string emission is already cheap, and its probes are already batched and prefetched. Deriving the integer parts per token (memo, vocabulary probe, unit) costs as much as the string emission saved.
- **The remaining exact route** would rebuild context fill so that unit ids come out of the work it already does. §5 showed that even the outer ceiling of all front-half string paths stops at ≈6.9× golden-v1 / 6.7× OSM.
- **Final answer:** under byte-identical output, **cpi (6.08× golden-v1, 5.93× novel, 5.78× OSM, validated in 121) remains the deliverable.** The rewrite track produced no measured gain.

## 9. Correction after experiment 151 (2026-10-05T14:56-0700)

§6's first bullet ("exact algebraic restructuring … is ruled out by byte-identity, because it changes floating-point order") is **wrong as a general statement**. Byte-identity applies to the output labels, not to the score bits. Experiment 151 rebuilt every original score bit-exactly from the oracles. It then bounded the error of reordered-double and float32 arithmetic (including the original's own rounding) and propagated the bounds through the original Viterbi. Results:
- every decision on the original path is strictly separated for **100% of golden-v1, novel and OSM addresses**;
- **99.94–99.99%** of fuzz addresses are certified; the remainder are 15 exact ties, which fall back.

A certified argmax with an exact fallback is therefore legal under hard rule 1. The 0.8–0.9 µs "fixed by byte-identity" bucket (141) is fixed only for *this* implementation strategy. §5's "no identified exact path reaches 7.8×" remains true for the *current architecture*. It does not bound a different architecture; see `experiments/151-optimality-bound/README.md` and `PLAN.md`.
