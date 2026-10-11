# Benchmark suite report — 202-fresh-v14-correctness-v1

Generated 2026-10-09T21:51:28-0700. Baseline `original-000` vs `cpi-packaged`. 3 rounds × 1800 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–1800). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.


## Per dataset: original-000 vs cpi-packaged

| dataset | source | addresses | original-000 µs | cpi-packaged µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|

## Correctness (byte-identical to the baseline)

- dumps: 1/1 identical (1,801 parsed addresses compared)
- frozen references: —
- fuzz: —
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Threads

- original-000: samples [1, 1, 0, 0, 0, 0] (max 1)
- cpi-packaged: samples [1, 1, 1, 1, 0, 0] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
