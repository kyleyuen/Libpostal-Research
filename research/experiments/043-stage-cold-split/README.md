# Experiment 043 — per-stage first-touch (cold) vs warm cost of `l3` (diagnostic)

Date/time/timezone: created 2026-10-02T17:10:09-0700; runs 2026-10-02T17:10:16-0700 to 2026-10-02T17:11:05-0700, America/Los_Angeles.
Status: complete (measurement only)

## Method
Harness `tools/bench_split.c` (no library change) linked against the existing stats builds `033 l3cf` (stage + context-fill timers) and `038 fs` (stage + feature-section timers), each with its own PGO profile, LTO and mimalloc. After one untimed pass, every address of the golden-v1 development split is parsed twice in a row for 10 passes; stage timers are accumulated separately for the first (cold) and second (warm) call. Two runs per build, AC-gated, all exit 0 (`raw/run.log`, `raw/split_*`).

## Results (run 1; run 2 totals within 0.01 µs)
| stage | cold µs | warm µs | first-touch |
|---|---:|---:|---:|
| total parse (cf build / fs build) | 8.244 / 8.408 | 3.731 / 3.831 | 4.51 / 4.58 |
| normalize | 0.073 | 0.045 | 0.03 |
| tokenize | 0.237 | 0.135 | 0.10 |
| context fill | 2.253 | 0.568 | 1.69 |
| · token normalization | 0.284 | 0.220 | 0.06 |
| · address-dictionary search | 0.279 | 0.062 | 0.22 |
| · prefix/suffix search | 0.220 | 0.110 | 0.11 |
| · component-phrase search | 0.999 | 0.106 | 0.89 |
| · postal-code search | 0.459 | 0.058 | 0.40 |
| features | 2.113 / 2.266 | 0.882 / 0.982 | 1.23 / 1.28 |
| · component block | 0.345 | 0.200 | 0.15 |
| · postcode block | 0.317 | 0.049 | 0.27 |
| · word block | 0.398 | 0.148 | 0.25 |
| · prev-word block | 0.213 | 0.134 | 0.08 |
| · next-word block | 0.721 | 0.221 | 0.50 |
| lookups | 1.213 | 0.494 | 0.72 |
| accumulation | 1.117 | 0.508 | 0.61 |
| Viterbi and rest of predict | 1.005 | 0.958 | 0.05 |
| response | 0.211 | 0.123 | 0.09 |

## Interpretation
First-touch cost ≈4.5 µs is spread over phrase search (1.29), feature generation (1.23), lookups+accumulation (1.33) and the rest; warm compute is led by Viterbi+rest (0.96), feature generation (0.88), lookups+accumulation (1.0) and context fill (0.57). Reaching ≈4.3 µs needs large cuts in several of these at once.
