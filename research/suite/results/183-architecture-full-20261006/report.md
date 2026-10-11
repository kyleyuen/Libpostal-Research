# Benchmark suite report — 183-architecture-full-20261006

Generated 2026-10-06T12:23:36-0700. Baseline `original-000` vs `cpi`, `release-candidate`. 5 rounds × 20 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–20). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi: 5.70× (geometric mean with each source weighted equally)**; by source: senzing 6.06× (1 set), senzing-style 5.42× (1 set), openstreetmap 5.64× (11 sets). Unweighted over all 13 datasets: 5.65× (range 5.42–6.06×; dominated by the OSM sets, which share one generator).
**release-candidate: 1.13× (geometric mean with each source weighted equally)**; by source: senzing 1.14× (1 set), senzing-style 1.17× (1 set), openstreetmap 1.07× (11 sets). Unweighted over all 13 datasets: 1.08× (range 1.04–1.17×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi

| dataset | source | addresses | original-000 µs | cpi µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | 12,863 | 32.253 | 5.341 | **6.17×** | 5.94–6.41 | 6.06× | 5.99–6.44 | 5.40× | 4.66 / 5.96 / 7.13 (n=81) |
| novel | senzing-style | 12,863 | 32.919 | 6.057 | **5.51×** | 5.16–5.90 | 5.42× | 5.27–5.99 | 5.31× | 4.06 / 5.31 / 6.19 (n=81) |
| osm-v3 | openstreetmap | 11,405 | 20.159 | 3.521 | **5.85×** | 5.32–6.44 | 5.73× | 5.45–6.68 | 5.35× | 4.64 / 5.87 / 6.57 (n=44) |
| unseen-v4 | openstreetmap | 10,293 | 19.248 | 3.298 | **5.57×** | 5.27–5.90 | 5.70× | 5.26–5.84 | 5.11× | 4.47 / 5.90 / 6.75 (n=41) |
| unseen-v5 | openstreetmap | 8,794 | 18.693 | 3.293 | **5.63×** | 5.46–5.79 | 5.59× | 5.51–5.85 | 5.23× | 4.69 / 5.94 / 6.92 (n=35) |
| unseen-v6 | openstreetmap | 9,450 | 18.419 | 3.309 | **5.73×** | 4.74–6.93 | 5.53× | 4.92–7.38 | 4.93× | 4.37 / 5.46 / 6.04 (n=36) |
| unseen-v7 | openstreetmap | 8,973 | 18.655 | 3.240 | **5.77×** | 4.77–6.97 | 5.63× | 4.80–7.33 | 4.86× | 4.53 / 5.75 / 6.73 (n=37) |
| unseen-v8 | openstreetmap | 9,981 | 18.185 | 3.227 | **5.43×** | 4.92–6.00 | 5.62× | 4.71–5.68 | 5.28× | 4.40 / 5.54 / 6.22 (n=40) |
| unseen-v9 | openstreetmap | 9,480 | 19.228 | 3.385 | **5.68×** | 5.35–6.03 | 5.70× | 5.26–6.00 | 5.14× | 4.30 / 5.81 / 6.85 (n=37) |
| unseen-v10 | openstreetmap | 8,567 | 18.136 | 3.429 | **5.41×** | 4.98–5.88 | 5.45× | 4.87–5.78 | 4.94× | 4.27 / 5.49 / 6.07 (n=36) |
| unseen-v11 | openstreetmap | 7,882 | 18.504 | 3.489 | **5.42×** | 4.85–6.07 | 5.61× | 4.77–6.03 | 4.81× | 4.64 / 5.51 / 5.94 (n=31) |
| unseen-v12 | openstreetmap | 8,273 | 18.066 | 3.150 | **5.82×** | 5.11–6.62 | 5.74× | 5.19–6.85 | 5.36× | 4.92 / 5.88 / 6.20 (n=34) |
| unseen-v13 | openstreetmap | 9,716 | 18.548 | 3.242 | **5.66×** | 5.44–5.89 | 5.69× | 5.46–5.92 | 5.09× | 4.56 / 5.71 / 6.48 (n=37) |

## Per dataset: original-000 vs release-candidate

| dataset | source | addresses | original-000 µs | release-candidate µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | 12,863 | 32.253 | 28.302 | **1.15×** | 1.13–1.17 | 1.14× | 1.14–1.18 | 1.14× | 1.11 / 1.15 / 1.22 (n=81) |
| novel | senzing-style | 12,863 | 32.919 | 28.491 | **1.16×** | 1.13–1.19 | 1.17× | 1.13–1.18 | 1.06× | 1.12 / 1.17 / 1.23 (n=81) |
| osm-v3 | openstreetmap | 11,405 | 20.159 | 18.531 | **1.08×** | 1.02–1.14 | 1.09× | 1.00–1.13 | 1.10× | 1.00 / 1.08 / 1.13 (n=44) |
| unseen-v4 | openstreetmap | 10,293 | 19.248 | 17.592 | **1.08×** | 1.05–1.11 | 1.07× | 1.05–1.11 | 1.07× | 0.99 / 1.04 / 1.11 (n=41) |
| unseen-v5 | openstreetmap | 8,794 | 18.693 | 18.068 | **1.03×** | 0.93–1.15 | 1.04× | 0.93–1.17 | 1.09× | 0.92 / 0.98 / 1.04 (n=35) |
| unseen-v6 | openstreetmap | 9,450 | 18.419 | 17.039 | **1.14×** | 0.99–1.31 | 1.08× | 1.05–1.39 | 1.15× | 1.07 / 1.12 / 1.17 (n=36) |
| unseen-v7 | openstreetmap | 8,973 | 18.655 | 17.148 | **1.09×** | 1.03–1.15 | 1.07× | 1.05–1.18 | 1.07× | 1.04 / 1.08 / 1.15 (n=37) |
| unseen-v8 | openstreetmap | 9,981 | 18.185 | 17.025 | **1.06×** | 1.02–1.10 | 1.07× | 1.00–1.08 | 1.14× | 1.00 / 1.06 / 1.11 (n=40) |
| unseen-v9 | openstreetmap | 9,480 | 19.228 | 17.834 | **1.10×** | 1.02–1.18 | 1.07× | 1.06–1.22 | 1.08× | 1.05 / 1.07 / 1.13 (n=37) |
| unseen-v10 | openstreetmap | 8,567 | 18.136 | 16.576 | **1.07×** | 1.03–1.11 | 1.07× | 1.04–1.13 | 1.01× | 1.03 / 1.07 / 1.13 (n=36) |
| unseen-v11 | openstreetmap | 7,882 | 18.504 | 19.709 | **1.01×** | 0.88–1.14 | 1.08× | 0.88–1.09 | 1.01× | 0.93 / 0.99 / 1.03 (n=31) |
| unseen-v12 | openstreetmap | 8,273 | 18.066 | 18.095 | **1.06×** | 0.94–1.21 | 1.04× | 0.96–1.26 | 1.13× | 1.02 / 1.06 / 1.10 (n=34) |
| unseen-v13 | openstreetmap | 9,716 | 18.548 | 17.027 | **1.11×** | 1.05–1.17 | 1.08× | 1.07–1.19 | 1.09× | 1.03 / 1.08 / 1.11 (n=37) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,614,198,272 | 2,613,397,736 | 2.77 | 2492 MB |
| cpi | 2,543,108,096 | 2,543,405,432 | 11.61 | 1927 MB |
| release-candidate | 2,615,934,976 | 2,615,118,056 | 2.77 | 2492 MB |

## Correctness (byte-identical to the baseline)

- dumps: 26/26 identical (257,080 parsed addresses compared)
- frozen references: cpi/golden-v1: RESULT: IDENTICAL; release-candidate/golden-v1: RESULT: IDENTICAL; cpi/novel: RESULT: IDENTICAL; release-candidate/novel: RESULT: IDENTICAL
- fuzz: cpi/s1: identical (2,000,000 inputs); release-candidate/s1: identical (2,000,000 inputs); cpi/s2: identical (2,000,000 inputs); release-candidate/s2: identical (2,000,000 inputs)
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Threads

- original-000: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi: samples [1, 1, 1, 1, 1, 1] (max 1)
- release-candidate: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
