# Experiment 145 — deletion bound of the bigram string path in cpi (diagnostic, quantitative ceiling for the rewrite's main card)

Date/time/timezone: created 2026-10-04T20:27:34-0700 (`raw/created.txt`), America/Los_Angeles. Commands: `raw/commands.txt`.
Status: complete (diagnostic). Kill rule not triggered: ceiling 7.1% dev / 9.1% OSM ≥ 1.5%

## Hypothesis, model and kill rule (written before building or measuring)

User request 2026-10-04: plan the rewrite through math rather than build blindly.

**Model.** For the bigram card (C1 + C3), the saving per address is

    ΔT = n_b · (c_build + c_lookup_str − c_probe_int) − n_tok · c_unit,

where:
- n_b is the bigram keys per address (123: 9.6 development);
- c_build is the cost of assembling one string key;
- c_lookup_str is cpi's string dictionary probe;
- c_probe_int is the integer pair-table probe;
- c_unit is the per-token unit-id cost.

The terms with a minus sign cannot be negative, so ΔT ≤ ΔT_max = n_b · (c_build + c_lookup_str): the whole cost of the bigram string path.

**Measurement of ΔT_max.**
- The variant `nbs` is cpi + `EXP_NO_BIGRAM` (both bigram emission sites skipped) + `EXP_STAGE_STATS`.
- It is compared with 119's `css` (cpi + `EXP_STAGE_STATS`) by the same split harness on development, OSM and v11.
- ΔT_max = Δ(features + lookups), cold and warm.
- Accumulate and Viterbi deltas are **excluded**. Deleting features also removes their weight-row accumulation, which an exact rewrite still performs.
- Output is deliberately **not** identical; this variant is never a candidate. A small difference is that unchanged code downstream may take different branches on the changed labels; the split harness attributes time by stage, which contains this.

**Kill rule (declared now).** If ΔT_max(features + lookups, cold) / total_cold < 1.5% on **both** development and OSM, the bigram rewrite cannot pass M3's ≥ +1.5% threshold even with free integer probes. The integer-pipeline track then stops before M2 code is built.

## Reproduction

- Sources: `final/119-cpi` with the two bigram emission lines wrapped in `#ifndef EXP_NO_BIGRAM` (`raw/address_parser.diff`: 4 lines added).
- Build `nbs` (`raw/switches_nbs.txt`) via 119's `build_cc.sh` (PGO/LTO/mimalloc), exit 0. `bench_split_vt.c` was linked the same way as `css`; hashes in `artifacts/nbs/hashes.txt`.
- Output differs from the original by design: golden-v1 sha `13bfcb3e…`, and 843 of 12,863 parses changed (`raw/output_note.txt`).
- Timing: `tools/run.sh`, nbs alternating with 119 `css`, development / OSM / v11, 4 rounds, 3 passes, gated, 2026-10-04T20:37:55 → 20:43:23 (`raw/timing_start.txt`, `raw/DONE`). All 24 runs exit 0; hygiene in `raw/hygiene.txt`, load ≈ 2.5.
- Analysis: `raw/analyze.stdout`. Model: `tools/model.py` → `raw/model.stdout`.

## Results (split harness, µs per address, median of 4; nbs − css)

| | dev cold / warm | OSM cold / warm | u11 cold / warm |
|---|---|---|---|
| features | −0.171 / −0.099 | −0.160 / −0.087 | −0.148 / −0.082 |
| lookups | −0.220 / −0.060 | −0.170 / −0.054 | −0.153 / −0.050 |
| **ΔT_max = features + lookups** | **0.391 (7.1%) / 0.160 (6.5%)** | **0.330 (9.1%) / 0.141 (6.6%)** | **0.301 (8.9%) / 0.132 (6.1%)** |
| accumulate (excluded: an exact rewrite keeps these adds) | −0.054 / −0.022 | −0.047 / −0.019 | −0.044 / −0.018 |
| context fill, Viterbi | within ±0.04 | within ±0.04 | within ±0.04 |

The per-run totals do not overlap (dev css 5.50–5.56 vs nbs 4.95–5.12 cold), so the effect is far above the ≈1% noise floor.

## Model (`tools/model.py`; every input measured, every output UNVERIFIED)

ΔT = c_build_saved + lookup_saving − key_compose − unit_cost, with:
- c_build_saved = the features delta above;
- lookup_saving = 122's in-place measurement of integer pair probes vs string probes on cpi: −0.032 dev / −0.035 OSM. cpi already batches and prefetches every dictionary probe, so the integer table removes only hashing and comparison, not the miss;
- unit_cost: 122's canonicalization lookups cost 13.0 / 10.3 ns each in place. That gives three cases:
  - pessimistic: one fresh lookup per token;
  - middle: 131's 38% already cached;
  - optimistic: free;
- key_compose ≈ 1 ns per key (assumption).

| | dev | OSM |
|---|---|---|
| pessimistic / middle / optimistic gain | 2.1% / 2.6% / 3.5% | 3.6% / 4.2% / 5.1% |
| projected speedup (121 base × 1/(1−f)) | 6.08× → 6.21 / 6.24 / 6.30× | 5.78× → 5.99 / 6.03 / 6.09× |
| deletion ceiling (not exact) | 6.54× | 6.35× |
| 7.8× needs | −22.1% (1.22 µs) | −26.0% (0.95 µs) |

## Decision

The bigram card passes its declared kill rule: the ceiling is ≥ 1.5%. Its realistic exact gain is ≈ 2–5%, so it would also clear M3's +1.5% threshold.

It cannot move the project toward 7.8×: even deleting bigrams outright, which is not exact, recovers only a third of the required time.

The overall derivation is in `research/analysis/REWRITE_MATH_2026-10-04.md`.

## Model v2 (after review; supersedes the table above)

`tools/coverage_122.py` measured 122's actual bigram coverage from the 142 oracles: c = 0.772 dev, 0.819 OSM. 122's terms were rescaled accordingly, and the phrase-type families now take 130's measured result (`tools/model2.py` → `raw/model2.stdout`).

| | dev | OSM |
|---|---|---|
| bigram card | 2.1–3.7% (6.21–6.31×) | 3.8–5.3% (6.00–6.10×) |
| whole pipeline | 4.5–6.9% (6.36–6.53×) | 5.3–7.5% (6.10–6.25×) |
| outer ceiling | 6.93× | 6.65× |

The deltas from 122 and 145 are each measured against cpi within their own session.
