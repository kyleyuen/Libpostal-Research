# Benchmark suite report — smoke-test

Generated 2026-10-04T22:34:11-0700. Baseline `original` vs `cpi`. 2 rounds × 3 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–3); the table shows the median over rounds, a bootstrap 95% CI over rounds (10000 resamples, seed 20261004), and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi: 6.24× geometric mean over 2 datasets** (range 5.56–7.02×); by source: openstreetmap 6.24×.

## Per dataset: original vs cpi

| dataset | source | addresses | original µs | cpi µs | **speedup ×** | 95% CI | min–max | cold × | countries min/median/max |
|---|---|---:|---:|---:|---:|---|---|---:|---|
| osm-v3 | openstreetmap | 11,405 | 24.890 | 3.548 | **7.02×** | 6.71–7.32 | 6.71–7.32 | 7.08× | 5.74 / 7.10 / 7.94 (n=49) |
| unseen-v11 | openstreetmap | 7,882 | 18.123 | 3.261 | **5.56×** | 5.50–5.61 | 5.50–5.61 | 5.17× | 2.61 / 5.74 / 8.10 (n=40) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original | 2,613,035,008 | 2,612,775,144 | 2.77 | 2492 MB |
| cpi | 2,543,124,480 | 2,543,438,224 | 11.62 | 1926 MB |

## Correctness (byte-identical to the baseline)

- dumps: 2/2 identical (19,287 parsed addresses compared)
- frozen references: —
- fuzz: cpi/s1: identical (2,000 inputs); cpi/s2: identical (2,000 inputs)
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Threads

- original: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
