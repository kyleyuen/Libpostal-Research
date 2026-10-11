# Benchmark suite report — 202-ac-unseen-v14-resumed-v1

Generated 2026-10-10T00:13:57-0700. Baseline `original-000` vs `cpi-packaged`. 3 rounds × 2300 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–2300). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-packaged: 5.72×** by the geometric mean of the three round ratios for this single unseen-v14 OSM dataset. The median of those paired round ratios is 5.72×; this run contains one dataset only.

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

- The suite sampler observed original-000 [1, 1, 0, 0, 0, 0] and cpi-packaged [1, 1, 1, 1, 0, 0]; zeros are samples after each short process had exited. The direct live-process supplement at `research/experiments/202-cpi-final-validation/raw/v14-thread-supplement-v1/manifest.json` recorded six live samples of one thread for each role.

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- This report covers only unseen-v14 (1,801 inputs); it does not include the other OSM sets or the Senzing datasets. The development split of golden-v1 trained PGO for the accepted CPI package.
