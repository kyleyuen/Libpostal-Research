# Benchmark suite report — 203-stream-v14-v1

Generated 2026-10-10T18:27:46-0700. Baseline `original-000` vs `cpi-stream-203`. 3 rounds × 2300 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–2300). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-stream-203: 5.72× (geometric mean with each source weighted equally)**; by source: openstreetmap 5.72× (1 set). Unweighted over all 1 datasets: 5.72× (range 5.72–5.72×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi-stream-203

| dataset | source | addresses | original-000 µs | cpi-stream-203 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| unseen-v14 | openstreetmap | 1,801 | 15.117 | 2.643 | **5.73×** | 5.68–5.78 | 5.72× | 5.72–5.76 | 5.37× | 5.16 / 5.66 / 6.15 (n=9) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,611,593,216 | 2,611,546,320 | 2.83 | — |
| cpi-stream-203 | 2,094,579,712 | 2,094,073,920 | 9.69 | — |

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
