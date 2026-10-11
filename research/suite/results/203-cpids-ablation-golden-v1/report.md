# Benchmark suite report — 203-cpids-ablation-golden-v1

Generated 2026-10-10T01:17:51-0700. Baseline `original-000` vs `cpi-119`, `cpi-119-no-cp-ids`. 3 rounds × 3 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–3). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-119: 6.19× (geometric mean with each source weighted equally)**; by source: senzing 6.19× (1 set). Unweighted over all 1 datasets: 6.19× (range 6.19–6.19×; dominated by the OSM sets, which share one generator).
**cpi-119-no-cp-ids: 5.93× (geometric mean with each source weighted equally)**; by source: senzing 5.93× (1 set). Unweighted over all 1 datasets: 5.93× (range 5.93–5.93×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi-119

| dataset | source | addresses | original-000 µs | cpi-119 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | 12,863 | 33.816 | 5.468 | **6.06×** | 5.36–6.85 | 6.19× | 5.73–6.28 | 5.70× | 5.04 / 6.13 / 7.21 (n=81) |

## Per dataset: original-000 vs cpi-119-no-cp-ids

| dataset | source | addresses | original-000 µs | cpi-119-no-cp-ids µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| golden-v1 | senzing | 12,863 | 33.816 | 5.692 | **5.92×** | 5.87–5.97 | 5.93× | 5.90–5.94 | 4.41× | 2.01 / 5.87 / 6.86 (n=81) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,614,067,200 | 2,613,233,872 | 2.67 | 2492 MB |
| cpi-119 | 2,537,046,016 | 2,537,343,352 | 11.68 | 1927 MB |
| cpi-119-no-cp-ids | 2,537,046,016 | 2,537,359,760 | 9.38 | 1878 MB |

## Correctness (byte-identical to the baseline)

- dumps: 2/2 identical (25,726 parsed addresses compared)
- frozen references: cpi-119/golden-v1: RESULT: IDENTICAL; cpi-119-no-cp-ids/golden-v1: RESULT: IDENTICAL
- fuzz: —
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Threads

- original-000: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi-119: samples [1, 1, 1, 1, 1, 1] (max 1)
- cpi-119-no-cp-ids: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
