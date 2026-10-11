# Benchmark suite report — 203-stream-gold-novel-v1

Generated 2026-10-10T17:20:09-0700. Baseline `original-000` vs `cpi-stream-203`. 3 rounds × 200 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–200). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-stream-203: 5.99× (geometric mean with each source weighted equally)**; by source: senzing 6.04× (1 set), senzing-style 5.93× (1 set). Unweighted over all 2 datasets: 5.99× (range 5.93–6.04×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi-stream-203

| dataset | source | addresses | original-000 µs | cpi-stream-203 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | 12,863 | 31.860 | 5.283 | **6.11×** | 5.81–6.42 | 6.04× | 6.03–6.25 | 5.84× | 4.96 / 6.36 / 7.66 (n=81) |
| novel | senzing-style | 12,863 | 32.367 | 5.456 | **5.83×** | 5.24–6.49 | 5.93× | 5.55–6.02 | 4.46× | 4.65 / 5.84 / 6.80 (n=81) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,614,149,120 | 2,613,348,584 | 2.80 | — |
| cpi-stream-203 | 2,111,258,624 | 2,110,326,824 | 9.86 | — |

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
