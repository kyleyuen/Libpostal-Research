# Benchmark suite report — 202-final-senzing-v2

Generated 2026-10-09T22:05:46-0700. Baseline `original-000` vs `cpi-packaged`. 3 rounds × 200 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–200). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-packaged: 5.95× (geometric mean across the two Senzing datasets)**; by source: senzing 6.03× (golden-v1), senzing-style 5.87× (novel). Unweighted over these two datasets: 5.95× (range 5.87–6.03×). This run contains no OSM datasets.

## Per dataset: original-000 vs cpi-packaged

| dataset | source | addresses | original-000 µs | cpi-packaged µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | 12,863 | 31.453 | 5.213 | **6.07×** | 5.74–6.42 | 6.03× | 5.96–6.23 | 4.74× | 4.80 / 6.15 / 7.44 (n=81) |
| novel | senzing-style | 12,863 | 31.825 | 5.425 | **5.87×** | 5.78–5.95 | 5.87× | 5.83–5.90 | 3.99× | 4.40 / 5.93 / 7.06 (n=81) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,474,016,768 | 2,478,360,712 | 2.84 | 2172 MB |
| cpi-packaged | 2,537,046,016 | 2,537,343,352 | 11.70 | 1927 MB |

The maximum process peak footprint is 2,537,343,352 B for CPI versus 2,478,360,712 B for original (+58,982,640 B). This two-dataset group fails the peak-footprint comparison even though the reported golden-v1 steady footprint is lower. Peak includes process startup; observed peak variation is not attributed to the implementation or model.

## Threads

- original-000: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi-packaged: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- golden-v1's development split trained PGO. This report covers only golden-v1 and novel; OSM datasets are outside this run.
