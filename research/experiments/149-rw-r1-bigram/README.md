# Experiment 149 — REWRITE_PLAN M3: R-build of the integer bigram path on cpi (r1a, r1)

Date/time/timezone: created 2026-10-04T21:45:46-0700 (`raw/created.txt`), America/Los_Angeles. Commands: `raw/commands.txt`.
Status: complete — **rejected by declared rule 4** (no variant ≥ +1.5% on both screening sets); M3 validation and v12 not run; integer-pipeline track stopped

**Record gap, stated honestly.** This README was written *after* the builds and correctness checks but *before any timing run*. The design rationale below was fixed by experiments 147 and 148 before any code was written, and the decision rules are declared here before timing.

## Design (from 146, 147, 148)

- **Base.** cpi + `EXP_INT_KEYS_PAIRS` (122's batched, prefetched integer pair table; vocabulary-canonical parts). 147/148 showed it costs about as much as string emission: +0.084 µs canonicalization and +0.042 µs integer machinery against −0.135 µs string emission saved (warm, 300 dev addresses).
- **`EXP_CANON_RING`.** A per-address, content-validated cache of `fd_canon` (and of the pair unit) for strings that miss the token memos, so phrase strings are not canonicalized again and again. It is exact because a hit requires the same length and the same bytes. It is reset per address, so there is no cross-address memoization.
- **`EXP_PHRASE_UNITS`.** Ids ≥ the vocabulary dictionary size for the 528,327 non-vocabulary strings that occur as parts of single-bar model bigram keys (model-derived; built at load).
  - Pair keys with phrase parts are also moved to the integer table: 6,667,173 state keys are integer-routed, against 1,642,049 in cpi.
  - Parse time uses the same unit rule as load time (`ap_pair_part` = `fd_pair_unit` on the stripped part), so a pair is integer-routed if and only if its key was moved.
- **Population (§6).**
  - `r1a` = base + ring;
  - `r1` = base + ring + phrase units;
  - stage-timer twins `r1as` and `r1s`.

## Correctness (done before timing)

- r1a and r1 pass `tools/verify.sh`:
  - golden-v1 `45574ee2…` and novel `806b8a81…`;
  - OSM, unseen v4 and both frozen references identical;
  - fuzz seeds 1 and 2 (2 × 2M) identical.
- `tools/verify_unseen.sh`: unseen v5–v11 are identical to each set's original-library dump, for both variants.
  - A first attempt from zsh compared against a garbled path and reported a false "v5 DIFFERENT". It is kept in `raw/verify_r1/summary.txt` and marked invalid.
- The stage twins give golden-v1 `45574ee2…`.
- Load (`raw/r1_dict_verbose.stderr`): phrase units ok, ids 7,389,122–7,917,449 (< 8,388,607); string state dictionary 191.1 MB + int 228.9 MB, against cpi's 398.2 + 56.4 MB; phrase-unit table 24 MB + 8.3 MB arena.

## Decision rules (declared before timing)

1. **Choose the variant on development only.** Use the dev screen (golden-v1 + OSM, 6 rounds, `tools/screen_split.sh`) and the stage timers (dev, OSM).
2. **The chosen variant goes to the M3 milestone:**
   - the 121/103 protocol (original / cpi / candidate on golden-v1, novel and OSM, 5 rounds, thread check, steady footprint);
   - then unseen golden-v12.
3. **Stop the track** if any of these holds:
   - under 121/103 the candidate is not ≥ +1.5% on both golden-v1 and OSM;
   - OSM cold features + lookups fall by < 0.10 µs (split harness);
   - it is slower on v12;
   - its peak footprint is > original − 20 MB (2,594 MB).
4. **No candidate faster on the screen** (≥ +1.5% median on both sets, with a stage-timer mechanism): record it and stop; M3 is not run.

## Results

**Timing.**
- Screen 2026-10-04T22:00:03 → 22:09:58 (`raw/screen/`); split → 22:16:27 (`raw/split/`).
- 60 runs, all exit 0, gated, with load, swap and free memory logged per run (`raw/screen/load.log`). The settle wait required load < 2.5 before starting (`raw/screen/settle.log`).
- The load still rose to 3.4–4.7 during the runs.

**Screen** (bench_ext, 20 passes, 6 rotating rounds; per-round speedup = cpi / variant, median of passes 2–20):

| | golden-v1 median | rounds faster | OSM median | rounds faster | min-based (v1 / OSM) |
|---|---:|---:|---:|---:|---|
| r1a | 0.993× | 2/6 | 0.996× | 3/6 | 1.000 / 0.977 |
| r1 | 0.985× | 0/6 | 1.012× | 4/6 | 0.989 / 0.996 |

**Stage timers** (`raw/split_analyze.stdout`, median of 4).
- Cold deltas are contaminated: css dev total 6.43 µs vs 5.53 in 145, and context fill, which these switches do not touch, moved by −0.17…+0.14 µs.
- Warm deltas are consistent:

| warm Δ vs css | r1as dev | r1s dev | r1as OSM | r1s OSM |
|---|---:|---:|---:|---:|
| features | +0.035 | +0.064 | +0.035 | +0.053 |
| lookups | −0.013 | −0.016 | −0.009 | −0.013 |
| features + lookups | +0.022 | +0.047 | +0.026 | +0.041 |

## Interpretation

- **The ring cache did not remove the integer path's extra cost.** r1a's warm features delta (+0.035) equals 122's (+0.037), so the repeated fallthrough canonicalizations were not the dominant part of 148's canon group. The rest is the memo path itself (`ap_vm_get`, pointer checks, the vocabulary probe of the not-yet-filled next token) and the integer machinery.
- **Phrase units make it worse (+0.064 warm on dev).** More bigrams take the integer route, and each costs more than its string emission.
- **Conclusion: in cpi's architecture, emitting a bigram as an integer key costs more compute than emitting it as a string.** cpi's string emission is already cheap (length-known joins, 059/061/098), and its string probes are already batched and prefetched. The ceiling measured in 145 is therefore not reachable by this route. An exact design would have to remove the per-token vocabulary/memo work itself, i.e. restructure context fill, which is the larger and riskier front-half rewrite that REWRITE_MATH §5 already showed cannot reach 7.8×.
- Both variants are exact everywhere tested, and peak memory was not measured, because M3 was not reached. They are kept as evidence and are not carried forward.
