# Experiment 062 — exact all-letters fast path in token normalization (on 061 `nb2`)

Date/time/timezone: created 2026-10-02T20:06:29-0700, America/Los_Angeles. Status: **accepted into the stack** (`lf`)

Change (`EXP_LETTERS_FAST`, `normalize.c` `add_normalized_token`): a token whose bytes are all ASCII letters A–Z/a–z is appended in one piece. For such a token every branch of the per-character loop is inert (no hyphen, full stop, digit or apostrophe: no replacement, deletion, alpha/numeric split or possessive rule can fire) and each byte is appended unchanged, so the output bytes are identical for every option set.
Checks: outputs identical on golden-v1, novel, OSM, 2 × 2M fuzz (`raw/verify_summary.txt`).
Screen (6 rounds, `raw/screen_analyze.stdout`):
    v1 nb2 [6.869, 6.891, 6.868, 6.871, 6.866, 6.879] median 6.87
    v1 lf [6.841, 6.836, 6.83, 6.822, 6.834, 6.85] median 6.835
    v1 lf vs nb2 per round [1.0041, 1.008, 1.0056, 1.0072, 1.0047, 1.0042]
    osm nb2 [4.379, 4.333, 4.351, 4.368, 4.362, 4.366] median 4.364
    osm lf [4.328, 4.348, 4.304, 4.323, 4.318, 4.308] median 4.321
    osm lf vs nb2 per round [1.0118, 0.9966, 1.0109, 1.0104, 1.0102, 1.0135]
+0.4–0.8% golden-v1 (6/6 rounds), ≈+1% OSM (5/6).
