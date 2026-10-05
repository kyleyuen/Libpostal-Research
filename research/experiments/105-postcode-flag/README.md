# Experiment 105 — skip postcode-trie walks that a model-derived flag says fall off (EXP_PC_FLAG)

Date/time/timezone: created 2026-10-03T23:04:52-0700 (`raw/created.txt`), America/Los_Angeles; commands with timestamps in `raw/commands.txt`. Status: **inconclusive — exact, real but tiny saving; not carried forward**

## Change
Based on the 104 probe.
- **Load.** `ap_ftt_build` sets bit 60 of each vocabulary word's 056 entry when `trie_word_falls_off` says the postcode-trie walk of that word, from the root, takes the search loop's fall-off branch.
- **Context fill.** The admin slots from the component search are kept and passed to the postcode search (same admin tokens).
- **Search loop.** A flagged token in the fresh state (EXP_PHRASE_PREFILTER's condition, start node = root) skips the walk. It sets the exact state the fall-off path leads to: NO_MATCH, `phrase_start = phrase_len = 0`, node reset.
- **Housekeeping.** The dormant `EXP_FTT_VERIFY` comparison masks bits 60–62.
Diffs: `raw/address_parser.diff`, `raw/trie_search.diff`. Switches: `raw/switches_pcg.txt` = psf + `-DEXP_PC_FLAG`.

## Correctness
`pcv` (+ `EXP_PC_FLAG_VERIFY`: every postcode search re-run without flags and its phrases compared; + 101 and 056 verify modes) and `pcg`: all identical on golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2. About 1.7M searches per fuzz seed were compared with no abort (`raw/verify_*`).

## Speed
- **Screens 1–3** (`raw/screen*_analyze*.stdout`) are unusable: within-run drift from external CPU load (psf alone ranged 6.21–6.60 µs on golden-v1).
- **Machine cleanup**, then **screen 4**: two orphaned crash reporters were stopped (ACTIVITY_LOG). Screen 4 used 10 rounds and two statistics per run (median and minimum of passes 2–20; `tools/screen_analyze2.py`):
        v1 median psf 5.824 pcg 5.847; pcg speedup per round [0.9419, 0.9754, 1.0086, 1.0043, 1.0072, 1.0877, 0.9904, 0.9583, 1.1264, 0.8875] median 0.9973; rounds faster 5/10
        v1 min    psf 5.565 pcg 5.617; pcg speedup per round [0.9604, 0.9656, 1.0281, 0.9967, 1.0118, 1.0404, 0.9932, 0.9767, 1.1, 0.8989] median 0.995; rounds faster 4/10
        osm median psf 3.773 pcg 3.752; pcg speedup per round [1.1419, 0.9692, 1.0088, 1.0085, 0.996, 0.9968, 0.9904, 1.0035, 1.0181, 1.0] median 1.0017; rounds faster 5/10
        osm min    psf 3.63 pcg 3.609; pcg speedup per round [1.1143, 0.9749, 1.0315, 0.9953, 1.0092, 0.9809, 0.9881, 1.0039, 1.0377, 1.0081] median 1.006; rounds faster 6/10
  No detectable difference; per-round scatter is still about ±5%.
- **Stage timers.** Builds `pss` = psf and `gss` = pcg, from `sources_stats/` with a postcode-search timer in ss_t[12]. A first stats build failed, with its logs kept in `raw/stats_attempt1/`. 4 alternating runs (`raw/split_analyze.stdout`):

| µs per address, cold / warm | dev psf | dev pcg | OSM psf | OSM pcg |
|---|---:|---:|---:|---:|
| postcode search | 0.424 / 0.059 | 0.412 / 0.054 | 0.213 / 0.054 | 0.186 / 0.048 |
| context fill | 1.810 / 0.392 | 1.824 / 0.395 | 1.042 / 0.355 | 1.016 / 0.357 |
| total parse | 6.099 / 2.636 | 6.150 / 2.625 | 4.027 / 2.296 | 3.992 / 2.295 |

The postcode search is 0.01–0.03 µs faster, as intended. That is below 1% of the total, and total parse time does not move.

## Decision
Inconclusive, not carried forward, and golden-v9 is not spent on it. The skipped fall-off walks are cheap: their nodes are near the root and mostly cached. The postcode stage's 0.42 µs cold cost on dev lies elsewhere, probably in numeric tokens walking deep into the trie (104: unflagged tokens mostly walk in).
