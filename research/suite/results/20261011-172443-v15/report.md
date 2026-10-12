# Benchmark suite report — 20261011-172443-v15

Generated 2026-10-11T18:11:02-0700. Baseline `original` vs `optimized`. 3 rounds × 3 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–3). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**optimized: 4.94× (geometric mean with each source weighted equally)**; by source: reviewer 4.94× (1 set). Unweighted over all 1 datasets: 4.94× (range 4.94–4.94×; dominated by the OSM sets, which share one generator).

## Per dataset: original vs optimized

| dataset | source | addresses | original µs | optimized µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| v15 | reviewer | 7,266,489 | 14.632 | 2.958 | **4.96×** | 4.81–5.12 | 4.94× | 4.91–5.04 | 4.99× | 3.83 / 5.04 / 6.17 (n=88) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original | 3,023,765,504 | 3,022,326,280 | 2.65 | 2546 MB |
| optimized | 2,501,705,728 | 2,500,659,528 | 9.54 | 2314 MB |

## Correctness (byte-identical to the baseline)

- dumps: 1/1 identical (7,266,489 parsed addresses compared)
- frozen references: —
- fuzz: optimized/s1: identical (2,000,000 inputs); optimized/s2: identical (2,000,000 inputs)
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Threads

- original: samples [1, 1, 1, 1, 1, 1] (max 1)
- optimized: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
