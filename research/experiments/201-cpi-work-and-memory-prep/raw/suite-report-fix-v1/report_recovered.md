# Benchmark suite report — 201-token-storage-matched-golden-screen-v2

Generated 2026-10-09T02:17:44-0700. Baseline `original-000` vs `cpi-packaged`, `cpi-source-matched-o2`, `199-token-span-mimalloc-v2`. 3 rounds × 200 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–200). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-packaged: 6.09× (geometric mean with each source weighted equally)**; by source: senzing 6.09× (1 set). Unweighted over all 1 datasets: 6.09× (range 6.09–6.09×; dominated by the OSM sets, which share one generator).
**cpi-source-matched-o2: 5.75× (geometric mean with each source weighted equally)**; by source: senzing 5.75× (1 set). Unweighted over all 1 datasets: 5.75× (range 5.75–5.75×; dominated by the OSM sets, which share one generator).
**199-token-span-mimalloc-v2: 5.81× (geometric mean with each source weighted equally)**; by source: senzing 5.81× (1 set). Unweighted over all 1 datasets: 5.81× (range 5.81–5.81×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi-packaged

| dataset | source | addresses | original-000 µs | cpi-packaged µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | — | 32.208 | 5.286 | **6.03×** | 5.59–6.50 | 6.09× | 5.83–6.17 | 4.05× | 4.68 / 5.98 / 7.16 (n=81) |

## Per dataset: original-000 vs cpi-source-matched-o2

| dataset | source | addresses | original-000 µs | cpi-source-matched-o2 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | — | 32.208 | 5.604 | **5.55×** | 4.68–6.57 | 5.75× | 5.13–5.79 | 4.50× | 4.20 / 5.61 / 6.58 (n=81) |

## Per dataset: original-000 vs 199-token-span-mimalloc-v2

| dataset | source | addresses | original-000 µs | 199-token-span-mimalloc-v2 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | — | 32.208 | 5.558 | **5.82×** | 5.73–5.91 | 5.81× | 5.79–5.86 | 4.82× | 4.45 / 5.88 / 6.89 (n=81) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,613,444,608 | 2,612,709,584 | 2.80 | 2172 MB |
| cpi-packaged | 2,537,046,016 | 2,537,343,352 | 11.59 | 1927 MB |
| cpi-source-matched-o2 | 2,537,013,248 | 2,537,343,352 | 11.91 | 1927 MB |
| 199-token-span-mimalloc-v2 | 2,537,029,632 | 2,537,359,736 | 11.90 | 1927 MB |

## Threads

- original-000: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi-packaged: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi-source-matched-o2: samples [1, 1, 1, 1, 1, 1] (max 1)
- 199-token-span-mimalloc-v2: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
