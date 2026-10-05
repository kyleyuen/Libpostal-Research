# Experiment 147 — where 122's integer bigram path spends its time: feature sections of cpi + EXP_INT_KEYS_PAIRS (diagnostic, input to M3 design)

Date/time/timezone: created 2026-10-04T21:23:29-0700 (`raw/created.txt`); timing 2026-10-04T21:28:47-0700 → 21:34:55-0700, America/Los_Angeles. Status: complete (diagnostic)

## Hypothesis

The cost model (`research/analysis/REWRITE_MATH_2026-10-04.md`) has one large unknown, the unit-id cost. Experiment 122 (cpi + `EXP_INT_KEYS_PAIRS`) skipped building bigram strings, yet features rose by 0.079 µs cold on dev. So canonicalizing the bigram parts cost about 0.25 µs.

Hypothesis: that extra cost sits in the next-word section (fs_next_word), where the next token's vocabulary memo is filled ahead of time, and/or in the phrase parts.

Method:
- `ifs` = 144's `fss` sources and switches + `EXP_INT_KEYS_PAIRS`;
- timed alternating with 144's `fss` on development, OSM and v11 (4 rounds, 3 passes, split harness);
- each section compared.

No kill rule: this is a diagnostic. It decides where the M3 port must save unit-id cost.

## Reproduction

- `ifs` = 144 sources + `raw/switches_ifs.txt` (fss + `EXP_INT_KEYS_PAIRS`), built with 119's recipe; exit 0. Output: golden-v1 sha `45574ee2…` (`raw/identity.txt`).
- `tools/run.sh`: ifs alternating with 144's fss on development, OSM and v11, 4 rounds, 3 passes, gated; all 24 runs exit 0.
- Analysis: `raw/analyze.stdout`.

## Results

**This session was noisier than 144.** fss's total cold is 6.41 vs 5.58 µs on dev, and load was 3.6–3.9 at the start (`raw/hygiene.txt`). The cold deltas of stages that `EXP_INT_KEYS_PAIRS` does not touch moved by −0.06…−0.21 µs (context fill), so **cold deltas in this session are not usable**. Warm deltas are stable across the three sets.

| warm Δ (ifs − fss), µs | dev | OSM | u11 |
|---|---|---|---|
| features | +0.038 | +0.023 | +0.021 |
| of which prev-word section | +0.025 | +0.018 | +0.013 |
| of which next-word section | +0.012 | +0.005 | +0.006 |
| lookups | −0.013 | −0.014 | −0.011 |
| total | +0.021 | +0.008 | +0.019 |

## Interpretation

- In cpi's architecture, 122's integer bigram emission costs **more warm compute** than the string emission it replaces. Per bigram that is two `ap_canon_part` calls, `fdi_key`, and `crf_ik_push` into the batched integer lookup.
- The integer probes save only ≈0.011–0.014 µs warm.
- The extra cost sits mainly in the prev-word section, i.e. the canonicalization of the left part.
- This falsifies the optimistic end of the 145 model (string build saved for free) **for this design**.
- A new R-build only gains if it emits a bigram id with less work than cpi's string emission costs. From 145, that is ≈0.10 µs warm for ≈9.6 bigrams on dev, about 10 ns per bigram. It would therefore need per-token unit ids computed once and an inline probe.
