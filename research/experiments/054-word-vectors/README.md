# Experiment 054 — precomputed per-word leading state-score vectors (on 053 `pc`)

Date/time/timezone: created 2026-10-02T18:17:27-0700; screen 2026-10-02T18:20:18-0700 to 2026-10-02T18:22:05-0700, America/Los_Angeles.
Status: **rejected (no gain)**

## Change (switch `EXP_WORD_VEC`, `sources/address_parser.c` + `sources/crf.c`)
At load, for the 400,000 most frequent vocabulary words above the rare-word threshold (model counts only): three 20-double vectors = exact in-order sums of the weight rows of [postcode variant features], bias, prefix, suffix (as the address dictionary marks them for those bytes) and word (183.1 MB + 28.2 MB index). At parse time an eligible token (nothing emitted before its postcode block, word block runs, pre-normalization bytes equal the word, frequency above threshold, vector present) emits none of those features and its state row starts as a copy of the matching vector. `EXP_WORD_VEC_VERIFY` emits everything and checks bitwise that the running sum equals the vector at that point.

## Checks
Verify build: 6,466 (golden-v1) + 5,542 (novel) + 5,762 (OSM) token rows checked, **0 mismatches**. Timing build `wv`: golden-v1 output byte-identical (sha 45574ee2…).

## Screen (sequential, alternating, golden-v1, 20 passes, 4 rounds; `raw/screen_analyze.stdout`)
    pc [7.492, 7.498, 7.523, 7.526] median 7.511
    wv [7.544, 7.534, 7.532, 7.551] median 7.539
    wv vs pc per round [0.9931, 0.9952, 0.9988, 0.9967]

## Decision
Rejected: only ≈8% of tokens qualify (most tokens are inside address-dictionary or component phrases, or are numbers whose normalized form differs), and reading a cold 160-byte vector per eligible token offsets the saved lookups (−0.3 to −0.7% vs `pc`).
