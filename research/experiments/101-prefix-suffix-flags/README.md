# Experiment 101 — model-derived "prefix/suffix search returns nothing" flags in the 056 entries (EXP_PS_FLAGS, idea A)

Date/time/timezone: created 2026-10-03T22:18:32-0700; screen 22:21:04 → 22:26:22, screen 2 22:26:31 → 22:31:46, stage split finished 22:35:10, America/Los_Angeles. Status: **accepted** (unseen golden-v8 in 102: +3.1% vs npr, every round)

## Hypothesis and change
Context fill runs `search_address_dictionaries_prefix` and `_suffix` on every raw token (073: 0.22 µs cold / 0.11 warm). For about 65% of calls the raw bytes equal the admin-normalized bytes and form a vocabulary word; for those, the results are a fixed function of the word (100).
Change (`sources/address_parser.c`, `raw/address_parser.diff`):
- At load, `ap_ftt_build` (056) also runs both searches on every vocabulary key, with lang NULL as at the call site. It sets bit 61 / bit 62 of that word's FTT entry when the prefix / suffix result is NULL_PHRASE. These bits were unused; every reader masks its fields or tests bit 63 only; the dormant `EXP_FTT_VERIFY` comparison now masks them (`raw/trie_search.diff`). **No extra memory.**
- The prefix/suffix loop moves after the FTT slot loop and just before the component-phrase search. Nothing in between reads its arrays.
- For a token whose raw bytes equal its admin bytes and which has an FTT slot, a set flag replaces that search with NULL_PHRASE. Every other case runs the original search.
- The flags are derived from the model's vocabulary and dictionaries only; no benchmark data is used.
Base: npr (098); switches `raw/switches_psf.txt` = npr + `-DEXP_PS_FLAGS`.

## Correctness
Verify build `psv` (+ `EXP_PS_FLAGS_VERIFY`, which re-runs every skipped search and aborts unless it returns NULL_PHRASE, + `EXP_FTT_VERIFY`):
- Flagged searches re-run: 66,988 (one corpus), 76,294 (another), 15,004,988 and 15,021,184 (fuzz seeds), all with **0 mismatches**.
- FTT walks: 0 mismatches.
- Outputs identical (`raw/verify_psv/`).
`psf`: golden-v1, novel, OSM, unseen v4, frozen references, fuzz seeds 1 and 2 are **all identical** (`raw/verify_psf/summary.txt`).

## Screens (vs npr; sequential, rotating order, 20 passes, 6 rounds)
Screen 1 (`raw/screen_analyze.stdout`): golden-v1 rounds 1–4 ran slow for **both** binaries (npr 6.66–6.72 vs its usual 6.30); kept as recorded. OSM: +2.4%, 6/6 rounds.
Screen 2, a full repeat (`raw/screen2_analyze.stdout`):
    v1 npr [6.287, 6.294, 6.488, 6.309, 6.344, 6.316] median 6.312
    v1 psf [6.192, 6.199, 6.197, 6.201, 6.21, 6.212] median 6.2
    v1 psf speedup vs npr per round [1.0153, 1.0153, 1.047, 1.0174, 1.0216, 1.0167] median 1.0171
    osm npr [4.034, 4.025, 4.049, 4.037, 4.04, 4.047] median 4.038
    osm psf [3.929, 3.935, 3.903, 3.913, 3.907, 3.926] median 3.92
    osm psf speedup vs npr per round [1.0267, 1.0229, 1.0374, 1.0317, 1.034, 1.0308] median 1.0312
Machine note: before screen 2 (`raw/top_before_screen2.txt`), two VS Code `chrome_crashpad_handler` processes were using about 170% CPU each, `icdd` 99% and a temp process 78%. That load is not ours, and env_gate did not catch it. The binaries alternate, so the ratios are comparable; absolute times may be slightly inflated.
Peak RSS (bench_ext): identical to npr in every run (2,537,046,016 / 2,543,108,096 B). Setup: about 9.83 s vs 9.64 s (+0.2 s for the load-time pass).

## Mechanism (stage timers; `nss` = npr, `fss` = psf from this tree, EXP_STAGE_STATS; `raw/split_analyze.stdout`)

| µs per address, cold / warm | dev npr | dev psf | OSM npr | OSM psf |
|---|---:|---:|---:|---:|
| context fill | 2.033 / 0.463 | 1.919 / 0.415 | 1.244 / 0.419 | 1.129 / 0.370 |
| features | 1.552 / 0.817 | 1.527 / 0.805 | 1.045 / 0.668 | 1.019 / 0.658 |
| lookups, accumulate | unchanged | | unchanged | |
| total parse | 6.608 / 2.849 | 6.471 / 2.792 | 4.460 / 2.480 | 4.297 / 2.412 |

Context fill: −0.11 µs cold / −0.05 µs warm on both sets, as predicted from the share of searches skipped.

## Decision
Screen-positive with a confirmed mechanism; awaits the golden-v8 unseen check (measured together with 098's npr, which it stacks on).

Update: Accepted after the unseen check (102: psf vs npr +3.1%, per-round 1.024–1.040; psf 5.37× vs original). Validated as the new deliverable in 103.
