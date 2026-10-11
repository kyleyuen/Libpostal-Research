# Benchmark suite report — parallel-20261008-199-mimalloc-v2-memory

Generated 2026-10-07T21:55:11-0700. Baseline `original-000` vs `199-token-span-storage-mimalloc-v2`. 5 rounds × 20 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–20). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.


## Per dataset: original-000 vs 199-token-span-storage-mimalloc-v2

| dataset | source | addresses | original-000 µs | 199-token-span-storage-mimalloc-v2 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|

## Threads

- original-000: samples [1, 1, 1, 1, 1, 1] (max 1)
- 199-token-span-storage-mimalloc-v2: samples [1, 1, 1, 1, 1, 1] (max 1)

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
