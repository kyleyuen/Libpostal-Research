# Benchmark suite report — 203-stream-osm-v3-v13-v1

Generated 2026-10-10T18:20:56-0700. Baseline `original-000` vs `cpi-stream-203`. 3 rounds × 420 passes per dataset and variant, serial, gated, variant order rotated per round. **Speedup (×) = original-000 µs/address ÷ candidate µs/address** in the same round (warm = median of passes 2–420). The table shows the geometric mean of the per-round ratios with a 95% Student-t interval computed on their logarithms (rounds are the independent units), the median and the min–max over rounds. Cold = pass 1 only. Raw logs in `timing/`, environment per run in `env.txt`, hashes in `manifest.json`.

**cpi-stream-203: 5.55× (geometric mean with each source weighted equally)**; by source: openstreetmap 5.55× (11 sets). Unweighted over all 11 datasets: 5.55× (range 5.49–5.70×; dominated by the OSM sets, which share one generator).

## Per dataset: original-000 vs cpi-stream-203

| dataset | source | addresses | original-000 µs | cpi-stream-203 µs | **speedup ×** (geo-mean of rounds) | 95% CI (t, log ratios) | median | min–max | cold × | countries (≥50 addr) min/median/max |
|---|---|---:|---:|---:|---:|---|---:|---|---:|---|
| osm-v3 | openstreetmap | 11,405 | 18.825 | 3.305 | **5.72×** | 5.61–5.83 | 5.70× | 5.70–5.77 | 5.11× | 4.55 / 5.69 / 6.30 (n=44) |
| unseen-v4 | openstreetmap | 10,293 | 17.862 | 3.170 | **5.64×** | 5.61–5.66 | 5.63× | 5.63–5.65 | 4.75× | 4.40 / 5.77 / 6.56 (n=41) |
| unseen-v5 | openstreetmap | 8,794 | 17.789 | 3.212 | **5.52×** | 5.46–5.57 | 5.51× | 5.50–5.54 | 4.68× | 4.57 / 5.64 / 6.52 (n=35) |
| unseen-v6 | openstreetmap | 9,450 | 17.461 | 3.137 | **5.55×** | 5.50–5.59 | 5.55× | 5.53–5.57 | 4.90× | 4.39 / 5.45 / 6.19 (n=36) |
| unseen-v7 | openstreetmap | 8,973 | 17.519 | 3.163 | **5.53×** | 5.48–5.58 | 5.52× | 5.51–5.55 | 4.70× | 4.35 / 5.53 / 6.44 (n=37) |
| unseen-v8 | openstreetmap | 9,981 | 17.214 | 3.140 | **5.48×** | 5.40–5.56 | 5.49× | 5.44–5.50 | 4.96× | 4.29 / 5.47 / 6.12 (n=40) |
| unseen-v9 | openstreetmap | 9,480 | 18.074 | 3.264 | **5.54×** | 5.47–5.61 | 5.53× | 5.52–5.57 | 4.78× | 4.13 / 5.59 / 6.60 (n=37) |
| unseen-v10 | openstreetmap | 8,567 | 17.172 | 3.058 | **5.61×** | 5.54–5.69 | 5.61× | 5.59–5.65 | 4.87× | 4.37 / 5.79 / 6.49 (n=36) |
| unseen-v11 | openstreetmap | 7,882 | 17.528 | 3.182 | **5.51×** | 5.46–5.57 | 5.51× | 5.49–5.54 | 4.91× | 4.75 / 5.65 / 6.04 (n=31) |
| unseen-v12 | openstreetmap | 8,273 | 16.834 | 3.058 | **5.50×** | 5.48–5.52 | 5.50× | 5.49–5.50 | 4.75× | 4.95 / 5.84 / 6.23 (n=34) |
| unseen-v13 | openstreetmap | 9,716 | 17.212 | 3.111 | **5.52×** | 5.47–5.56 | 5.51× | 5.50–5.54 | 5.01× | 4.59 / 5.61 / 6.57 (n=37) |

## Memory and setup

| variant | max peak RSS (bytes) | max peak footprint (bytes) | setup s (median) | steady footprint |
|---|---:|---:|---:|---|
| original-000 | 2,613,362,688 | 2,613,053,648 | 2.77 | — |
| cpi-stream-203 | 2,108,129,280 | 2,107,476,008 | 9.62 | — |

## Limitations

- One laptop; load, swap, thermal and power state per run are in `env.txt`. Interactive load inflates cold numbers more than warm ones.
- The OSM sets share one generator; golden-v1's development split trained PGO. Reserved unseen sets are excluded unless `--include-reserved`.
