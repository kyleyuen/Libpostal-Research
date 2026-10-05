# Experiment 035 — measured ceiling: how fast could a restructured front half be? (replay oracle; CEILING PROBE ONLY)

Date/time/timezone: created 2026-10-02T15:57:52-0700; builds 15:58–15:59, timing 2026-10-02T15:59:35-0700 to 2026-10-02T16:08:51-0700, America/Los_Angeles.
Status: complete — **diagnostic ceiling, never a speed result.** The oracle memoizes each address's feature ids across calls, which METHOD forbids for results; it must never be packaged.

## Question and change

User goal (2026-10-02): find the ceiling of this architecture and aim for ~90% of it via restructuring. Probe: switch `EXP_ORACLE` in an experiment copy of the `l3` sources (`sources/`, base hashes `raw/base_sources.sha256`). On the first call for an input string (keyed by the caller's address pointer; `bench_ext` keeps one strdup per address), the normal path runs and the final per-token state/transition feature ids are recorded after the dictionary lookups (`crf.c`). On later calls the parser still normalizes, tokenizes, runs weight accumulation, Viterbi and builds the response, but skips context fill, feature generation and lookups and copies the recorded ids instead (`address_parser.c` skips `address_parser_context_fill` and clears the phrase arrays used by the single-phrase bypass).
PGO is not usable for the oracle (the PGO training harness `dump_outputs` reuses one line buffer, so pointer keys would collide), so both the oracle `ocn` and the control `l3n` are built **without PGO** (LTO, mimalloc, scanner -O1; `tools/build_cc.sh`); `l3n` sources verified byte-equal to `final/021-l3/sources`. The first `l3n` build failed on a wrong relative SRCDIR (`raw/build_l3n_attempt1_badpath.stderr`) and was rebuilt.

## Checks

Oracle counters (stderr of every `ocn` run): golden-v1 records 12,860 / replays 244,340; novel 12,854 / 244,226; OSM 11,405 / 216,695 — exactly 19 replays per recorded address (3 golden-v1 and 9 novel addresses take the single-phrase bypass and are never scored). Total output components per run identical for original, `l3n` and `ocn` on every dataset and round (golden-v1 1,100,740; novel 1,024,220; OSM 848,020). Labels were not compared directly (bench_ext counts components only).

## Results (sequential timing per 034; 5 rotating rounds, 20 passes, median of warm passes 2–20; `raw/analyze.stdout`; 45 runs exit 0)

| binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 34.104 | 1.000× (1.00–1.00) | 34.766 | 1.000× (1.00–1.00) | 21.586 | 1.000× (1.00–1.00) |
| l3 no-PGO control | 8.171 | 4.164× (4.15–4.26) | 8.251 | 4.210× (4.19–4.33) | 5.282 | 4.087× (4.01–4.31) |
| oracle ceiling (no PGO) — probe only | 2.625 | 12.986× (12.90–13.24) | 2.655 | 13.095× (13.08–13.21) | 1.914 | 11.403× (10.64–11.83) |

peak footprint bytes (max over rounds): orig 2613332176/2613446888/2613102824; l3n 2380351720/2380351720/2370308304; ocn 2385561832/2385561832/2375583976

**Ceiling** (front half — context fill, feature generation, lookups — free; back half as implemented today, no PGO): **2.63 µs (13.0×) golden-v1, 2.66 µs (13.1×) novel, 1.91 µs (11.4×) OSM.** The front half costs 5.55 / 5.60 / 3.37 µs of the no-PGO control. Replayed back-half time (2.63 µs) is close to the in-situ stage sum (tokenize+normalize+accumulate+Viterbi+response ≈ 2.6 µs in 033), so the front half does not noticeably slow the back half through cache eviction.
**90% of the ceiling** (user target) ≈ 11.7× / 11.8× / 10.3×, i.e. about 2.9 µs/address on golden-v1: the whole front half would have to run in ≈ 0.3 µs (today 5.5 µs). The ceiling rises further only if accumulation and Viterbi themselves get faster (paper floors in `analysis/LIMITS_2026-10-02.md`).

## Level 2: keys replayed, lookups still paid (added 16:15–16:29)

The level-1 oracle also skips the dictionary probes that turn each feature key into an id, which any legitimate design must still perform. Switch `EXP_ORACLE_KEYS` (same experiment copy; `crf.c` before this change kept as `raw/crf_c_before_keys_level.c`, from which `ocn` was built) records the final feature key arrays and integer keys after feature generation and, on later calls, restores them and runs the normal lookups, accumulation and Viterbi (context fill and feature generation skipped). Build `okn` (no PGO, LTO, mimalloc). Smoke test `raw/okn_smoke.*`: components 330,222 over 6 passes = 55,037 per pass, matching the original. Sequential timing `tools/run_timing2.sh` (2026-10-02T16:19:49 to 16:29:08, `raw/timing2_start.txt`, `raw/DONE2`; 45 runs exit 0; `raw/analyze2.stdout`; replay counts identical to level 1):

| binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
|---|---:|---:|---:|---:|---:|---:|
| original | 33.827 | 1.000× (1.00–1.00) | 34.535 | 1.000× (1.00–1.00) | 21.593 | 1.000× (1.00–1.00) |
| l3 no-PGO control | 8.174 | 4.159× (4.09–4.18) | 8.192 | 4.202× (4.16–4.24) | 5.272 | 4.103× (3.93–4.14) |
| level-2 ceiling: keys replayed, lookups paid (no PGO) — probe only | 3.905 | 8.696× (8.60–8.71) | 4.008 | 8.563× (8.51–8.70) | 2.854 | 7.584× (7.40–7.98) |

peak footprint bytes (max over rounds): orig 2615167208/2613561576/2613577936; l3n 2380351720/2380368128/2370308304; okn 2383972584/2383890688/2374453480

**Realistic ceiling (front half free except the dictionary lookups): 3.91 µs (8.70×) golden-v1, 4.01 µs (8.56×) novel, 2.85 µs (7.58×) OSM.** Lookups and their misses account for about 1.3 µs of the level-1/level-2 gap. 90% of this ceiling ≈ 7.8× / 7.7× / 6.8×, i.e. about 4.3 µs/address on golden-v1: context fill plus feature generation (today ≈4.3 µs) would have to shrink to ≈0.4 µs.

## Decision

Ceiling measured. Reaching ~90% of it requires replacing the front half with an integer pipeline (token ids once; phrase matching on ids; feature ids from model-derived tables; no feature strings), keeping feature emission order and trie-search semantics exact, and replacing (not adding to) the tries within the steady-memory margin. This is a multi-session engine rewrite, planned in phases with per-phase gates.
