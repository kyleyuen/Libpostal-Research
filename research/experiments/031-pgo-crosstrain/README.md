# Experiment 031 — PGO cross-training matrix (does PGO overfit to its training dataset?)

Date/time/timezone: record created 2026-10-02T14:20:11-0700 (`raw/created.txt`), America/Los_Angeles.
Status: complete — no PGO overfitting detected; PGO worth ~2–4%; new same-session steady-memory finding (see Memory)

## Hypothesis and change

Concern (user, 2026-10-02): no tuning to our datasets. The shipped 021 `l4` is built with whole-library PGO whose training run parses golden-v1 development addresses, so its code layout is derived from one of our evaluation sets. Output cannot change (PGO only changes code layout/inlining), but speed could be biased toward the training data. Test: rebuild the exact 021 `l4` sources, switches and toolchain with PGO trained on each of the three datasets in turn, plus one build with no PGO, and time every build on every dataset. Overfitting would show as a build being faster on its own training set (diagonal) than the other builds are there; off-diagonal cells are true held-out results.
No source change. Sources: `research/final/021-l4/sources` (the packaged deliverable). Build script `tools/build_cc.sh` = 021's with E=031, sources from the package, PGO training input from `$PGO_TRAIN`, profiles in `~/.cache/libpostal-031-pgo/` (diff shown in the session log; three lines changed).

## Builds

| Build | PGO training input | Same as l4 except |
|---|---|---|
| `l4` (021, shipped) | golden-v1 development (10,308) | — |
| `tv1` | golden-v1 all (`profiling/golden-v1/all.txt`, 12,863) | training input |
| `tnv` | golden-v2-novel (`profiling/golden-v2-novel/novel.txt`, 12,863) | training input |
| `tosm` | golden-v3-osm (`profiling/golden-v3-osm/osm.txt`, 11,405) | training input |
| `nopgo` | none | no PGO (still LTO, mimalloc, scanner -O1) |

## Reproduction

Command: `caffeinate -i /bin/bash tools/run.sh`; analysis `python3 tools/analyze.py`. Every step's command, timestamps, stdout/stderr and exit code in `raw/run.log` and `raw/<step>.*`; input/binary hashes `raw/input_hashes.txt`, `artifacts/*/hashes.txt`; environment `raw/env.txt`; gate log `raw/gate.log`.
Output check: each new build's `dump_outputs` vs the shipped `l4` dumps on all three datasets (`raw/output_check.txt`); `l4` itself was verified byte-identical to the original on golden-v1, novel and OSM in 021/025.
Timing: original (`015-integer-keys/artifacts/bench_ext_baseline`), `l4`, `tv1`, `tnv`, `tosm`, `nopgo` on golden-v1 (`golden-v2-novel/v1-all.tsv`), novel (`novel.tsv`) and OSM (`osm.tsv`); 5 rounds, binary order rotated per round, 20 passes each, no warm-up file; statistic = median of warm passes 2–20; speedup paired with the original in the same round; AC gate before every run.

## Results

Builds 14:20:40–14:24:32 (`raw/run.log`); all 12 build×dataset dumps byte-identical to `l4` (`raw/output_check.txt`). Timing attempt 1 aborted after one run because `cap()` overwrote the global binary count `n` (`raw/outer.stderr`: "division by 0"); the script still wrote DONE. Preserved as `raw/timing_attempt1_failed/` and `raw/attempt1_DONE_premature`. Attempt 2 (`tools/run_timing.sh`, local variables): 2026-10-02T14:25:13 to 14:44:11 (`raw/timing_start.txt`, `raw/DONE`), 90 runs all exit 0, AC gate passed before each; host load average 7.5–8.5 (`raw/env.txt`). Analysis `python3 tools/analyze.py` → `raw/analyze.stdout`:

| binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 33.959 | 1.000× (1.00–1.00) | 34.806 | 1.000× (1.00–1.00) | 21.324 | 1.000× (1.00–1.00) |
| 021 l4 (golden-v1 development) | 7.865 | 4.307× (4.29–4.39) | 7.967 | 4.369× (4.30–4.46) | 5.052 | 4.288× (4.17–4.33) |
| tv1 (golden-v1 all) | 7.834 | 4.319× (4.23–4.35) | 7.907 | 4.361× (4.29–4.61) | 5.017 | 4.294× (3.80–4.43) |
| tnv (novel) | 7.831 | 4.314× (3.81–4.37) | 7.893 | 4.410× (4.36–4.55) | 4.990 | 4.306× (4.19–4.39) |
| tosm (OSM) | 7.833 | 4.326× (4.23–4.39) | 7.986 | 4.358× (3.76–4.38) | 4.966 | 4.306× (4.25–4.42) |
| nopgo (no PGO) | 8.065 | 4.207× (4.19–4.24) | 8.131 | 4.281× (4.20–4.34) | 5.261 | 4.109× (3.97–4.13) |

peak footprint bytes (max over rounds): orig 2613086440/2613233920/2614446312; l4 2470709552/2470725960/2476755248; tv1 2470709552/2470709552/2476755248; tnv 2470693168/2470693168/2476755248; tosm 2470725960/2470709552/2476755248; nopgo 2470709552/2470725936/2476755248

Speedup = original/variant warm median within the same round, median over 5 rounds (range in brackets). v1 = golden-v1 all (12,863), nv = novel (12,863), osm = OpenStreetMap (11,405).

**Overfitting test.** For each dataset, compare the build trained on it (diagonal) with the builds that never saw it:
- golden-v1: `tv1` 4.319× vs `tnv` 4.314×, `tosm` 4.326× (diagonal not faster).
- novel: `tnv` 4.410× vs `tv1` 4.361×, `tosm` 4.358×, `l4` 4.369× (+1.1%, inside the round-to-round ranges, e.g. `tv1` 4.29–4.61).
- OSM: `tosm` 4.306× vs `tv1` 4.294×, `tnv` 4.306×, `l4` 4.288× (no difference).
No build is measurably faster on its own training set; the largest diagonal advantage (+1.1%) is within noise. **Held-out results** (build never saw that dataset): 4.31–4.33× golden-v1, 4.36–4.37× novel, 4.29–4.31× OSM — the same as the shipped `l4`.

**What PGO contributes.** `nopgo` 4.207× / 4.281× / 4.109× vs PGO builds ≈ 4.31–4.33× / 4.36–4.41× / 4.29–4.31×, i.e. PGO adds about 2.5% / 2% / 4.5%; which dataset trains it does not matter.

## Memory

**Peak footprint** (`time -l`, max over 5 rounds): every build ≈ 2,470.7 MB (golden-v1, novel) and 2,476.8 MB (OSM) vs original 2,613.1–2,614.4 MB, i.e. −142 / −137 MB. PGO changes nothing in memory (data structures are identical).

**Steady footprint** (`footprint -p` during a running 400/1000-pass job on golden-v1, after the matrix; `raw/steady/`): original **2,172 MB** at 12, 24, 36, 48 and 60 s and at 25 s in a separate run; `l4`, `tosm` and `nopgo` **2,250 MB** at every sample. So in this session `l4`'s steady footprint is **+78 MB (+3.6%) above the original**. In 021's session the same `l4` measured 2,250 MB vs original 2,492 MB, and 002 measured the original at 2,172 MB. `l4` (mimalloc) is stable at 2,250 MB across sessions; the original moves between 2,172 and 2,492 MB, most likely depending on whether the macOS allocator keeps ~320 MB of freed load-time blocks dirty (023 found the same mechanism for the system-allocator `pt` build; UNVERIFIED for the original). The original's footprint here is 2,110 MB dirty MALLOC_LARGE + 59 MB MALLOC_SMALL; `l4`'s is 2,005 MB mimalloc regions + 243 MB untagged VM (`raw/steady/*.footprint`).
Consequence: "steady memory below the original" holds only against the original's higher state; against its lower state `l4` is 78 MB above. The likely cause is the sparse feature dictionaries (load factor 0.25, `EXP_LOADF=4`, chosen in 021 for speed; 021 `m3` at 0.5 was 4.25×). Not changed here.

**Method check and same-session comparison of 021 dictionary variants (14:56–15:09).** `tools/steady_check.sh`: original 2,171/2,172 MB and `l4` 2,250/2,250 MB with and without a warm-up file; the JSON `footprint` field agrees (`raw/steady/method_check.txt`, `json_footprint.txt`). `l4` reads exactly 021's recorded 2,250 MB, so the tool is consistent and the original's 2,492 MB in 021 was a different machine state, not a different metric. Under the project's standard (022 was rejected for steady memory above the original), **`l4` fails the steady-memory rule in this session.** `tools/steady_variants.sh` (preserved 021 binaries, same session; attempt 1 failed on unquoted paths, kept in `raw/steady/variants_attempt1_failed/`): `raw/steady/variants.txt`

| build (021) | feature-dict load factor | 021 speedup golden-v1 / novel / OSM | steady footprint today | vs original 2,172 MB |
|---|---|---|---:|---:|
| original | — | 1× | 2,172 MB (twice) | — |
| `s3` | 0.75 | 4.05–4.08× / 4.07× / 4.04–4.08× | 1,868 MB | −304 MB |
| `m3` | 0.5 | 4.25–4.28× / 4.31× / 4.24–4.25× | 2,026 MB | −146 MB |
| `l3` | 0.33 | 4.30× / 4.35× / 4.27× | 2,141 MB | −31 MB |
| `l4` (shipped) | 0.25 | 4.33× / 4.36× / 4.31× | 2,250 MB | **+78 MB** |

Speedups are from 021's controlled sessions (different session from these footprints). The steady footprint tracks the dictionary load factor, confirming the hypothesis above for these builds.

## Decision

1. PGO does not overfit: training on golden-v1, novel or OSM gives the same speed on every dataset, so held-out results equal the shipped `l4` numbers. To remove any dataset-derived step from the deliverable, a successor package can use PGO trained on a dataset that is not evaluated against (e.g. `tosm` for golden-v1/novel claims) with no speed cost; `l4` itself is unchanged.
2. Memory: the peak claim stands. `l4` fails the steady-memory rule against the original's low state (+78 MB). Existing builds `l3` (−31 MB, ~0.5% slower) and `m3` (−146 MB, ~1.5% slower) pass it in this session. Choice of successor deliverable put to the user.
