# Experiment 032 — final same-session validation of 021 `l3` as the deliverable

Date/time/timezone: record created 2026-10-02T15:13:42-0700; run 2026-10-02T15:13:47-0700 to 15:26:38 (`raw/start.txt`, `raw/DONE`), America/Los_Angeles.
Status: **accepted — `l3` is the new deliverable** (`research/final/021-l3/`)

## Why

031 found `l4`'s steady footprint (2,250 MB) 78 MB above the original's (2,172 MB) in a same-session check, failing the steady half of the memory rule; `l3` (same sources, feature dictionaries at load factor 1/3) measured 2,141 MB. The user chose `l3`. No new build: the preserved 021 `l3` binaries are validated as they are.

## Reproduction

Command: `caffeinate -i /bin/bash tools/validate.sh`; analysis `python3 tools/analyze.py` (031's analyzer restricted to original/l3/l4) → `raw/analyze.stdout`. Every step's command, timestamps and exit code: `raw/run.log`, `raw/<step>.*`; input hashes `raw/input_hashes.txt`; environment `raw/env.txt` (AC, load average 6.7–7.5); gate `raw/gate.log` (all OK). Binaries: original `015-integer-keys/artifacts/bench_ext_baseline`; `l3` and `l4` from `021-final-stack/artifacts/`.
Script defect: the summary lines that read exit codes, frozen results and thread counts used unquoted paths with spaces, so `raw/summary.txt` has empty fields (`raw/outer.stderr`). All underlying checks ran; `raw/summary_corrected.txt` restates them from the raw files.

## Results

**Correctness** (`raw/summary_corrected.txt`): `l3` binaries match 021's recorded hashes; output byte-identical to the original on golden-v1, novel and OSM, both frozen references identical, 2 × 2,000,000 fuzzed inputs identical; one thread at all six samples.

**Speed** (5 rotating rounds, 20 passes, median of warm passes 2–20, paired with the original in each round; 45 runs, all exit 0):

| binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 34.460 | 1.000× (1.00–1.00) | 34.027 | 1.000× (1.00–1.00) | 21.596 | 1.000× (1.00–1.00) |
| 021 l3 (deliverable) | 7.817 | 4.376× (4.30–4.44) | 7.935 | 4.332× (4.23–4.36) | 5.022 | 4.279× (4.14–4.31) |
| 021 l4 (previous) | 7.781 | 4.373× (4.31–4.46) | 7.933 | 4.323× (4.19–4.36) | 4.915 | 4.341× (4.28–4.43) |

peak footprint bytes (max over rounds): orig 2613659880/2613610728/2613020904; l3 2380335336/2380351744/2370308328; l4 2470709552/2470709552/2476755248

**Memory.** Peak footprint (time -l, max over rounds): `l3` 2,380.4 / 2,380.4 / 2,370.3 MB vs original 2,613.7 / 2,613.6 / 2,613.0 MB (−233 MB). Steady footprint (`raw/steady/steady.txt`, footprint -p at pass 3, two alternating samples each): original 2,172 / 2,172 MB, `l3` 2,141 / 2,141 MB (−31 MB), `l4` 2,250 / 2,250 MB (+78 MB). Load time: `l3` 8.5–8.7 s vs original 2.9–3.0 s.

## Decision

Accepted. `l3` meets every rule in this session: byte-identical output, one thread, peak and steady footprint below the original; speed equals `l4` within noise (golden-v1 4.38× vs 4.37×, novel 4.33× vs 4.32×, OSM 4.28× vs 4.34×). Packaged as `research/final/021-l3/` (sources identical to 021-l4, BUILD.md, build_hashes.txt, SHA256SUMS). Steady-memory margin is small (31 MB); `m3` (−146 MB, ~1.5% slower in 021) remains the fallback. `l4` stays preserved but is no longer the deliverable.
