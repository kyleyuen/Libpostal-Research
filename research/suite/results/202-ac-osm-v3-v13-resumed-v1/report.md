# Benchmark suite report — 202-ac-osm-v3-v13-resumed-v1

Generated 2026-10-10T00:07:22-0700. Baseline `original-000` vs `cpi-packaged`. 3 rounds × 420 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–420). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-packaged: 5.79× (geometric mean with each source weighted equally)**; by source: openstreetmap 5.79× (11 sets). Unweighted over all 11 datasets: 5.79× (range 5.53–6.09×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi-packaged

| dataset | source | addresses | original-000 µs | cpi-packaged µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| osm-v3 | openstreetmap | 11,405 | 20.292 | 3.464 | **5.87×** | 5.68–6.07 | 5.86× | 5.80–5.96 | 6.12× | 4.68 / 5.98 / 6.63 (n=44) |
| unseen-v4 | openstreetmap | 10,293 | 19.783 | 3.404 | **5.73×** | 4.95–6.65 | 5.81× | 5.37–6.04 | 3.34× | 4.56 / 5.70 / 6.50 (n=41) |
| unseen-v5 | openstreetmap | 8,794 | 19.646 | 3.287 | **6.04×** | 5.51–6.62 | 6.04× | 5.82–6.26 | 7.41× | 4.74 / 5.99 / 6.98 (n=35) |
| unseen-v6 | openstreetmap | 9,450 | 18.976 | 3.226 | **5.76×** | 5.51–6.03 | 5.71× | 5.69–5.88 | 4.42× | 4.56 / 5.74 / 6.50 (n=36) |
| unseen-v7 | openstreetmap | 8,973 | 19.244 | 3.355 | **5.79×** | 5.58–6.00 | 5.75× | 5.74–5.89 | 6.55× | 4.69 / 5.89 / 6.66 (n=37) |
| unseen-v8 | openstreetmap | 9,981 | 18.528 | 3.283 | **5.65×** | 5.47–5.83 | 5.69× | 5.57–5.70 | 6.47× | 4.33 / 5.30 / 5.96 (n=40) |
| unseen-v9 | openstreetmap | 9,480 | 19.391 | 3.497 | **5.57×** | 5.18–5.98 | 5.53× | 5.43–5.74 | 4.51× | 4.43 / 5.67 / 6.48 (n=37) |
| unseen-v10 | openstreetmap | 8,567 | 19.127 | 3.142 | **6.13×** | 5.80–6.47 | 6.09× | 6.02–6.28 | 5.62× | 5.10 / 6.54 / 7.42 (n=36) |
| unseen-v11 | openstreetmap | 7,882 | 18.452 | 3.319 | **5.53×** | 4.60–6.65 | 5.69× | 5.09–5.85 | 4.35× | 4.63 / 5.44 / 5.98 (n=31) |
| unseen-v12 | openstreetmap | 8,273 | 18.607 | 3.095 | **5.82×** | 5.33–6.36 | 5.80× | 5.63–6.05 | 4.92× | 4.83 / 5.67 / 6.00 (n=34) |
| unseen-v13 | openstreetmap | 9,716 | 18.665 | 3.234 | **5.70×** | 5.49–5.92 | 5.73× | 5.61–5.77 | 5.06× | 4.67 / 5.97 / 6.82 (n=37) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,613,100,544 | 2,613,905,616 | 2.90 | 2492 MB |
| cpi-packaged | 2,543,108,096 | 2,543,405,432 | 11.71 | 1926 MB |

## Threads

- original-000: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi-packaged: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
