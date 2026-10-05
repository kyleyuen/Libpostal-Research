# Benchmark suite report — 20261004-223647

Generated 2026-10-04T23:07:23-0700. Baseline `original` vs `cpi`. 5 rounds × 20 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–20). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi: 5.84× (geometric mean with each source weighted equally)**; by source: senzing 6.07× (1 set), senzing-style 5.94× (1 set), openstreetmap 5.54× (9 sets). Unweighted over all 11 datasets: 5.62× (range 5.41–6.07×; dominated by the OSM sets, which share one generator).

## Per dataset: original vs cpi

| dataset | source | addresses | original µs | cpi µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | 12,863 | 32.870 | 5.133 | **6.15×** | 5.97–6.34 | 6.07× | 6.05–6.40 | 5.63× | 5.36 / 6.57 / 7.69 (n=81) |
| novel | senzing-style | 12,863 | 31.585 | 5.343 | **5.99×** | 5.69–6.30 | 5.94× | 5.66–6.29 | 5.80× | 4.51 / 5.93 / 6.98 (n=81) |
| osm-v3 | openstreetmap | 11,405 | 18.719 | 3.332 | **5.56×** | 5.36–5.76 | 5.59× | 5.35–5.75 | 5.03× | 4.49 / 5.75 / 6.34 (n=44) |
| unseen-v4 | openstreetmap | 10,293 | 18.022 | 3.205 | **5.65×** | 5.48–5.83 | 5.64× | 5.48–5.86 | 5.03× | 4.39 / 5.69 / 6.43 (n=41) |
| unseen-v5 | openstreetmap | 8,794 | 18.548 | 3.426 | **5.29×** | 4.82–5.80 | 5.47× | 4.74–5.71 | 5.13× | 4.43 / 5.49 / 6.49 (n=35) |
| unseen-v6 | openstreetmap | 9,450 | 17.929 | 3.288 | **5.38×** | 4.80–6.03 | 5.41× | 4.73–6.09 | 5.13× | 4.32 / 5.37 / 6.04 (n=36) |
| unseen-v7 | openstreetmap | 8,973 | 17.889 | 3.252 | **5.49×** | 5.35–5.64 | 5.50× | 5.32–5.62 | 5.11× | 4.36 / 5.49 / 6.39 (n=37) |
| unseen-v8 | openstreetmap | 9,981 | 17.544 | 3.201 | **5.58×** | 5.31–5.85 | 5.48× | 5.44–5.98 | 4.99× | 4.31 / 5.44 / 6.13 (n=40) |
| unseen-v9 | openstreetmap | 9,480 | 18.105 | 3.292 | **5.62×** | 5.32–5.94 | 5.55× | 5.47–6.08 | 4.97× | 4.21 / 5.67 / 6.64 (n=37) |
| unseen-v10 | openstreetmap | 8,567 | 17.303 | 3.081 | **5.67×** | 5.46–5.88 | 5.61× | 5.54–5.97 | 5.23× | 4.70 / 6.07 / 6.72 (n=36) |
| unseen-v11 | openstreetmap | 7,882 | 18.028 | 3.217 | **5.75×** | 5.28–6.26 | 5.60× | 5.45–6.45 | 5.13× | 4.72 / 5.60 / 6.04 (n=31) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original | 2,614,247,424 | 2,613,430,480 | 2.75 | 2492 MB |
| cpi | 2,543,108,096 | 2,543,405,432 | 11.58 | 1927 MB |

## Correctness (byte-identical to the baseline)

- dumps: 11/11 identical (110,551 parsed addresses compared)
- frozen references: cpi/golden-v1: RESULT: IDENTICAL; cpi/novel: RESULT: IDENTICAL
- fuzz: cpi/s1: identical (2,000,000 inputs); cpi/s2: identical (2,000,000 inputs)
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Threads

- original: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
