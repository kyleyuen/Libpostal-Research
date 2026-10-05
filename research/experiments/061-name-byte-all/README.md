# Experiment 061 — name-byte keys for every feature helper (on 059 `nb`)

Date/time/timezone: created 2026-10-02T19:55:08-0700, America/Los_Angeles. Status: **accepted into the stack** (`nb2`)

Change: the feature-name macros (019's `FA_NAME_LEN`, 059's `EXP_NAME_BYTE`) moved above `add_phrase_features` and `add_ngram_features` in `address_parser.c`, so "phrase type+phrase" (≈6/address) and "unambiguous phrase type(+phrase)" (≈4/address) also use the 1-byte name form (060 showed them still on the text path). No other change.
Checks: outputs byte-identical to the original on golden-v1, novel, OSM and 2 × 2,000,000 fuzz (`raw/verify_summary.txt`).
Screen: first 4-round attempt was disturbed (single rounds 7.5 µs; kept in `raw/screen_attempt1_noisy/`, host load average ≈9); rerun with 6 rounds (`raw/screen_analyze.stdout`):
    v1 nb [7.022, 7.015, 7.021, 7.006, 7.037, 7.329] median 7.021
    v1 nb2 [7.012, 6.865, 6.864, 6.892, 6.861, 6.878] median 6.872
    v1 nb2 vs nb per round [1.0014, 1.0218, 1.0229, 1.0165, 1.0257, 1.0656]
    osm nb [4.502, 4.603, 4.495, 4.508, 4.541, 4.524] median 4.516
    osm nb2 [4.386, 4.356, 4.378, 4.37, 4.39, 4.365] median 4.374
    osm nb2 vs nb per round [1.0264, 1.0567, 1.0267, 1.0316, 1.0344, 1.0364]
`nb2` vs `nb`: +2.2% golden-v1 (median), +3.2% OSM; faster in 6/6 OSM rounds and 5/6 golden-v1 rounds (one tie).
