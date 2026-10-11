# Benchmark suite report — 203-stream-output-v1

Generated 2026-10-10T17:07:28-0700. Baseline `original-000` vs `cpi-stream-203`. 3 rounds × 200 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–200). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.


## Per dataset: original-000 vs cpi-stream-203

| dataset | source | addresses | original-000 µs | cpi-stream-203 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|

## Correctness (byte-identical to the baseline)

- dumps: 14/14 identical (130,341 parsed addresses compared)
- frozen references: cpi-stream-203/golden-v1: RESULT: IDENTICAL; cpi-stream-203/novel: RESULT: IDENTICAL
- fuzz: cpi-stream-203/s1: identical (2,000,000 inputs); cpi-stream-203/s2: identical (2,000,000 inputs)
- negative control (a one-byte change is detected): True
- **mismatches: none**

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
