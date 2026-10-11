# Benchmark suite report — 20261010-234430-v15

Generated 2026-10-10T23:48:02-0700. Baseline `original` vs `optimized`. 1 rounds × 3 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–3). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**optimized: 5.74× (geometric mean with each source weighted equally)**; by source: reviewer 5.74× (1 set). Unweighted over all 1 datasets: 5.74× (range 5.74–5.74×; dominated by the OSM sets, which share one generator).

## Per dataset: original vs optimized

| dataset | source | addresses | original µs | optimized µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| v15 | reviewer | 300 | 14.631 | 2.547 | **5.74×** | — | 5.74× | 5.74–5.74 | 4.08× | 6.07 / 6.07 / 6.07 (n=1) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original | 2,610,462,720 | 2,610,694,352 | 2.70 | 2490 MB |
| optimized | 2,090,467,328 | 2,090,174,528 | 9.72 | 1941 MB |

## Correctness (byte-identical to the baseline)

- dumps: 1/1 identical (300 parsed addresses compared)
- frozen references: —
- fuzz: optimized/s1: identical (20,000 inputs); optimized/s2: identical (20,000 inputs)
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Threads

- original: samples [1, 0, 0, 0, 0, 0] (max 1)
- optimized: samples [1, 1, 1, 0, 0, 0] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
