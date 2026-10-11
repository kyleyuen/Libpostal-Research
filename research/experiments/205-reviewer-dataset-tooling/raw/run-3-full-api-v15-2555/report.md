# Benchmark suite report — 20261010-235244-v15-tooling-check

Generated 2026-10-11T00:07:58-0700. Baseline `original` vs `optimized`. 3 rounds × 1566 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–1566). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**optimized: 6.91× (geometric mean with each source weighted equally)**; by source: reviewer 6.91× (1 set). Unweighted over all 1 datasets: 6.91× (range 6.91–6.91×; dominated by the OSM sets, which share one generator).

## Per dataset: original vs optimized

| dataset | source | addresses | original µs | optimized µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| v15-tooling-check | reviewer | 2,555 | 32.260 | 4.658 | **7.35×** | 5.59–9.66 | 6.91× | 6.88–8.34 | 3.46× | 6.89 / 6.89 / 6.89 (n=1) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original | 2,255,831,040 | 2,612,611,280 | 3.18 | 2171 MB |
| optimized | 2,031,943,680 | 2,110,277,672 | 10.23 | 1945 MB |

## Correctness (byte-identical to the baseline)

- dumps: 1/1 identical (2,555 parsed addresses compared)
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
