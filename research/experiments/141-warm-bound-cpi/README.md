# Experiment 141 — warm bound and cold/warm stage split of cpi (diagnostic)

Date/time/timezone: created 2026-10-04T19:00:16-0700 (`raw/created.txt`), America/Los_Angeles. Run times: see `raw/run.log`.
Status: complete (diagnostic). Timing 2026-10-04T19:01:37-0700 → 19:12:28-0700 (`raw/timing_start.txt`, `raw/DONE`); 48 runs, all exit 0; gate never failed.

## Hypothesis and change

Hypothesis (written before any run): 088 measured the warm bound on pcp (≈5.1×) at 11.1× golden-v1 / 8.1× OSM / 7.7× unseen v6. Since then
091/098/114/119 removed compute as well as misses, so on cpi the warm bound should still be ≈11–12× on golden-v1 development and ≈8–9× on OSM
and an unseen OSM set (UNVERIFIED expectation, from §7 of HANDOFF_2026-10-04 which mixed harnesses). If the OSM warm bound is below ≈9×, then
7.8× on OSM needs the warm compute itself cut, which sizes the integer-feature-pipeline rewrite (plans/REWRITE_PLAN). We also expect the
byte-identity-fixed part (warm accumulate + warm Viterbi) to be ≈0.9 µs warm on development (§4: 0.47 + 0.43), i.e. a hard floor no front-half rewrite can touch.

Change: none to the library (diagnostic). Three existing harnesses, unchanged sources:
- `bench_ext` (research/tools/bench_ext.c): normal µs/address, for the original (`015-integer-keys/artifacts/bench_ext_baseline`) and cpi (`119-component-phrase-ids/artifacts/cpi/bench_ext`).
- `bench_warm` (041-warm-bound/tools/bench_warm.c, 088's method): each address parsed once untimed, then again timed; linked here against cpi's own `libpostal.a` (`tools/bench_warm_cpi`).
- `bench_split` (119's `css` = cpi + EXP_STAGE_STATS, `tools/bench_split_vt.c`): per-stage cold/warm µs.

Datasets: golden-v1 development (025 TSV, 10,308), OSM golden-v3 (11,405), spent unseen golden-v11 (7,882). golden-v12 is reserved and not used.
Incremental comparison base: original libpostal (baseline bench_ext).

Analysis rules declared before running:
- Speedup = original bench_ext / cpi bench_ext (same harness). Warm bound = original bench_ext / cpi bench_warm (088's cross-harness ratio, labelled as such).
- Stage shares are taken from bench_split only and are never divided by bench_ext totals.
- Three buckets: front half = context_fill + features + lookups; fixed-under-identity = accumulate + viterbi_and_rest; other = normalize + tokenize + response.
  Only the warm part of the fixed bucket is pinned by float order; its cold−warm part is memory and `viterbi_and_rest` also contains score-matrix memset/memmove (132).
- Dev numbers are not comparable with 121's 6.08× (that was golden-v1 all).

## Reproduction

- Hardware/OS/compiler: Apple M4 Pro (8P+4E), 24 GB, macOS (Darwin 25.5.0), Apple clang 17.0.0 (`raw/clang_version.txt`); AC power 100%; `caffeinate` 21869 running.
- Machine hygiene (`raw/hygiene.txt`): `icdd` was at 72% CPU again and was restarted (`killall icdd`, 18:58:15) as on 2026-10-03; Chrome, Terminal and the
  Claude app stayed open (user active). Load averages 2.0–3.8 during the run (`raw/load.log`). Limitation: the per-run top-3 CPU column in `load.log` is empty
  (single `top -l 1` sample reads 0%); only load averages are informative.
- Inputs (`raw/identity.txt`): development = `025-audit-next/data/development.tsv` (column 2 is byte-identical to `profiling/golden-v1/development.txt`, `cmp` exit 0;
  10,308), OSM `golden-v3-osm/osm.tsv` (11,405), unseen `golden-v11-osm-unseen/unseen11.tsv` (7,882, already spent in 134). SHA-256 in `raw/identity.txt`.
- Binaries: cpi `bench_ext`, cpi/css `libpostal.a`, css `bench_split` and the original `bench_ext_baseline` match their recorded hashes (119 `artifacts/*/hashes.txt`,
  015 `baseline_bench_hashes.txt`). New links `tools/bench_warm_cpi` (cpi `libpostal.a`, `-flto`, mimalloc, cpi PGO profile) and `tools/bench_warm_orig`
  (000-baseline `libpostal.a`) by `tools/link.sh`; hashes in `raw/hashes.txt`. Link attempt 1 failed (exit 127, zsh did not word-split; `raw/attempt1_link_*`).
- Run: `tools/run.sh` (serial; gate before every run; 3 rounds × 3 datasets × 4 rotated variants, 20 passes, `/usr/bin/time -l`; then cpi stage-timer build 4 runs × 3 datasets, 3 passes).
  Analysis: `tools/analyze.py` → `raw/analyze.stdout` (per run: median of passes 2–20; then median over runs).
- Sanity check of the new link: `bench_warm_cpi`'s first-call µs (dev 5.56, OSM 3.59, u11 3.42) lands near cpi `bench_ext` and the split harness's cold totals,
  so the LTO relink did not produce materially different code.

## All measured results (µs/address)

| Set | original bench_ext | cpi bench_ext | cpi warm (088) | original warm | cpi speedup | **warm bound** (orig / cpi warm) | 7.8× needs |
|---|---:|---:|---:|---:|---:|---:|---:|
| golden-v1 development | 31.141 [30.53–31.15] | 5.103 [5.09–5.60] | 2.421 [2.40–2.44] | 13.534 | 6.10× | **12.86×** [12.99 12.75 12.61] | 3.992 |
| OSM golden-v3 | 18.429 [18.43–18.87] | 3.803 [3.44–4.05] ⚠ | 2.134 [2.13–2.14] | 12.051 | 4.85× ⚠ | **8.64×** [8.86 8.64 8.62] | 2.363 |
| unseen golden-v11 | 18.228 [17.63–19.84] | 3.270 [3.20–3.38] | 2.145 [2.14–2.16] | 11.909 | 5.57× | **8.50×** [9.20 8.22 8.51] | 2.337 |

⚠ **Interference.** Several cpi `bench_ext` runs show step changes inside the run, a sign of an external load rather than the code:
- dev r2 jumps from 5.1 to 5.7 µs at pass 12;
- u11 r2 jumps from 3.2 to 4.1 µs at pass 15;
- OSM r2 sits at about 4.0 µs throughout.

The OSM normal figure (3.80 µs, 4.85×) is therefore contaminated. Use 121's clean validation (OSM 3.334 µs, 5.78×) for cpi's normal speed. The warm-bound runs are tight (spread ≤ 1%), so the warm bound is robust.
Same-harness warm/warm (original warm / cpi warm): 5.59× dev, 5.65× OSM, 5.55× u11. cpi's advantage over the original is about the same with or without cold misses.

Peak RSS (max over runs): original 2.613–2.614 GB, cpi 2.538–2.543 GB (`raw/analyze.stdout`). Steady footprint not measured (diagnostic; 121 has it).

## Stage split (cpi + EXP_STAGE_STATS, split harness, µs/address cold / warm; median of 4 runs)

| Stage | dev | OSM | u11 |
|---|---|---|---|
| normalize + tokenize + response (**other**) | 0.498 / 0.285 | 0.404 / 0.249 | 0.366 / 0.274 |
| context fill | 1.723 / 0.375 | 0.990 / 0.345 | 0.843 / 0.336 |
| features | 1.375 / 0.687 | 0.879 / 0.567 | 0.845 / 0.593 |
| lookups | 0.668 / 0.201 | 0.415 / 0.167 | 0.385 / 0.172 |
| **front half** (cf + features + lookups) | **3.766 / 1.263** (68% / 51%) | **2.284 / 1.079** (61% / 50%) | **2.074 / 1.101** (60% / 50%) |
| accumulate | 0.807 / 0.471 | 0.610 / 0.441 | 0.567 / 0.420 |
| Viterbi and rest | 0.475 / 0.428 | 0.419 / 0.384 | 0.407 / 0.378 |
| **fixed bucket** (accumulate + Viterbi) | **1.282 / 0.900** (23% / 37%) | **1.030 / 0.825** (28% / 38%) | **0.974 / 0.798** (28% / 36%) |
| total | 5.566 / 2.465 | 3.736 / 2.169 | 3.437 / 2.191 |

Buckets sum to the total within 0.02 µs.

Two caveats on the fixed bucket:
- Only its **warm** part (≈0.80–0.90 µs) is pinned by byte-identity (float order). Its cold−warm part (accumulate 0.15–0.34 µs) is memory, which 114 already attacked.
- "Viterbi and rest" also holds the score-matrix memset/memmove (132), which identity does not pin.

## Correctness and interpretation

Exact output comparison: not applicable. No library change; the binaries are the hash-verified 119 artifacts already checked in 119/121.

What the numbers mean:
- **Hypothesis confirmed for OSM (8.6×) and unseen (8.5×); development exceeded the predicted 11–12× range (12.9×).** All three are above 088's pcp bound (11.1 / 8.1 / 7.7×), because 091–119 also cut compute.
- **7.8× sits only 9–10% above the warm bound on OSM-style data.**
  - On u11 (one session): 2.337 µs needed against a 2.145 µs warm bound. Memory-side work alone would have to remove ≈83% of the first-touch gap (3.270 → 2.337 of 1.125 µs).
  - On OSM, 141's original gives 18.429 / 7.8 = 2.363 µs needed against 2.134 warm. 141's cpi OSM normal figure is contaminated, so the cpi side of the gap is taken from 121's session (needs 2.48 = 19.32 / 7.8 vs cpi 3.334). The two sessions' numbers are never mixed in one ratio.
- **The front half is the largest bucket:** 60–68% cold and 50–51% warm in every set. The identity-fixed warm compute is ≈0.8–0.9 µs per address (36–38% of warm time). No rewrite of the front half can go below it.
- **Sizing, in split-harness proportions** (UNVERIFIED as a prediction). 7.8× needs:
  - dev −22% (141 session) → ≈1.22 µs of the 3.77 µs cold front half (32%);
  - OSM −25.7% (121 session) → ≈0.96 µs of 2.28 (42%);
  - u11 −28.5% (141 session) → ≈0.98 µs of 2.07 (47%).
  Each assumes nothing else changes.
- **Split-run spread** (`raw/split/*`): one cold outlier per set, total_parse cold 5.82 (dev r2), 4.58 (OSM r1) and 3.81 (u11 r1), against ≈5.5 / 3.7 / 3.42 for the others. The warm values are tight (±1.5%). The median of 4 absorbs one bad run per set, but cold shares carry that uncertainty.
- **Model hashes:** `raw/model_hashes.txt` (SHA-256 of every file under `LIBPOSTAL_DATA_DIR`, taken after the run).

Known limitations:
- The interference above.
- The split harness reads ≈9–12% above `bench_ext` cold, so only proportions transfer.
- Three rounds, not five.
- `bench_ext` passes are short (cpi ≈1 s per pass on development): METHOD's ≥ 10 s per trial is met per run, not per pass, the same as 121.

## Decision

Diagnostic complete. It sizes the rewrite plan: `research/plans/REWRITE_PLAN_2026-10-04.md` §2.
Ledger, activity log and handoff §12 were updated 2026-10-04.
