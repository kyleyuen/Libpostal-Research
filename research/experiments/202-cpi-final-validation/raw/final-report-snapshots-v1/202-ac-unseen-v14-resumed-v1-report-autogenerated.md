# Benchmark suite report — 202-ac-unseen-v14-resumed-v1

Generated 2026-10-10T00:13:57-0700. Baseline `original-000` vs `cpi-packaged`. 3 rounds × 2300 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–2300). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-packaged: 5.72× (geometric mean with each source weighted equally)**; by source: openstreetmap 5.72× (1 set). Unweighted over all 1 datasets: 5.72× (range 5.72–5.72×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi-packaged

| dataset | source | addresses | original-000 µs | cpi-packaged µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| unseen-v14 | openstreetmap | 1,801 | 15.271 | 2.672 | **5.69×** | 5.51–5.89 | 5.72× | 5.61–5.76 | 4.14× | 5.18 / 5.77 / 6.22 (n=9) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,611,281,920 | 2,611,349,712 | 2.76 | 2490 MB |
| cpi-packaged | 2,517,024,768 | 2,517,322,104 | 11.71 | 1926 MB |

## Threads

- original-000: samples [1, 1, 0, 0, 0, 0] (max 1)
- cpi-packaged: samples [1, 1, 1, 1, 0, 0] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
