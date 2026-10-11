# Experiment 154 — PLAN A3: engineered exact back half (bound, not a candidate; no library change)

Date/time/timezone: created 2026-10-05 (`raw/created.txt`), America/Los_Angeles. Runs in `raw/replay/run.log` (≈15:21–15:23:18).
Status: **complete (diagnostic)**. Per A3's rule, the projected ceiling is lowered.

## Hypothesis and change

PROOF.md §6 (experiment 152) says that adding rows in the original emission order is bit-exact under any layout. That holds even with SIMD
across labels and with Viterbi vectorized over j. A3 asks how far an **exact f64** back half can be pushed toward 151's ≈0.36 µs compute
floor. That floor assumed float32 and 4 lanes.

Change: `tools/replay_k3.c` is 151's replay kernel plus **K3**:
- the 35 model rows that cover all 20 labels are added densely (they serve ≈21% of state hits);
- each transition row carries a model-derived 20-bit mask of the prev-label rows it touches. Only those rows are zeroed per token; untouched rows point at a shared zero row;
- Viterbi is vectorized across j using the original operations.

All tables come from the model alone.

Rule, declared in PLAN before the run: there is no kill rule. If warm K3 is above 0.6 µs on golden-v1, the projected ceiling must be lowered.

## Reproduction

- `tools/run.sh`: the original `libpostal.a`, `-O3 -mcpu=native`, gated, serial, 3 rounds × golden-v1/OSM, 60 passes. All 6 runs exit 0, 0 gate failures. Hashes are in `raw/replay/hashes.txt`.
- **METHOD shortfall.** Processes ran 10.7–11.4 s (golden-v1) and 5.6–6.5 s (OSM) including load, shared across 4 variants. No single variant had ≥10 s of measured parsing.

## Results (`raw/replay/*.stdout`; µs/address; cold = median of passes 2–60, warm = marginal repeat cost)

| | golden-v1 cold (r1 / r2 / r3) | golden-v1 warm | OSM cold | OSM warm |
|---|---|---|---|---|
| K0 original | 3.59 / 3.27 / 3.57 | 1.64 / 1.79 / 1.81 | 2.24 / 2.22 / 2.22 | 1.63 / 1.70 / 1.72 |
| K1 packed f64 | 2.66 / 2.59 / 2.72 | 0.98 / 0.98 / 1.07 | 1.31 / 1.41 / 1.33 | 1.00 / 1.08 / 1.07 |
| K2 certified f32 | 1.96 / 2.11 / 2.39 | 0.67 / 0.62 / 0.69 | 0.97 / 1.00 / 0.98 | 0.82 / 0.69 / 0.76 |
| **K3 exact f64 engineered** | **2.45 / 2.40 / 2.36** | **0.81 / 0.98 / 0.83** | **1.15 / 1.80⚠ / 1.17** | **0.86 / 0.79 / 0.87** |

⚠ OSM r2 K3 has a wide spread (1.38–5.64 µs across passes), which points to interference.

Labels are identical to K0 on every address, for every variant, in every round.

## Interpretation

- **K3 is exact with no certificate.** It is 8–12% faster cold and ≈15% faster warm than K1.
- **Warm K3 is ≈0.83 µs on golden-v1, above 0.6 µs, so the projection must be lowered.** At the stated rates (f64 2-lane NEON at ≈3 vector ops per cycle; scatter at ≈1.5 cycles per add):
  - exact f64 Viterbi needs about 4.9k vector operations, ≈0.37 µs;
  - the ≈1,090 scatter adds take ≈0.37 µs;
  - so the exact back half has a compute floor of **≈0.6–0.75 µs**, not 0.36.

  K3 is close to that.
- **Float32 (K2) remains ≈20% faster warm.** It needs the A1 certificate and an f64 fallback copy, which A2 showed does not fit memory. It is therefore not adopted.
- **Revised projection (UNVERIFIED).** Taking K3 in place of K2 adds ≈0.4 µs cold on golden-v1 and ≈0.2 µs on OSM. A4's budgets are recomputed in experiment 155 from same-session measurements.

## Decision

Diagnostic complete. The back half to carry forward is K3 (exact f64, emission order). Ledger and activity log updated.
