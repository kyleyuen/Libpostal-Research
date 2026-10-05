# Experiment 109 — transitions per fresh token in the postcode-trie search, by token class (diagnostic)

Date/time/timezone: created 2026-10-04T00:06:03-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: complete

Build `pw` = psf + `EXP_PW_PROBE` counters (`sources/trie_search.c`), no PGO; dump_outputs on development and OSM (`raw/probe_*.stderr`). Development, per class: fresh tokens / transitions / walk length 1..8+.

    digits       16,406 / 51,461 / 2589 4792 1902 2332 4499 289 1 2
    alnum mixed  31,577 / 89,828 / 383 10841 16280 2570 476 975 3 49
    other ASCII   1,686 /  6,218
    non-ASCII    10,503 / 27,918

About 17 transitions per address. Digit tokens often walk 4–5 steps (real postcodes); letter words mostly 2–3, and 105 showed those near-root reads are cheap. → 111 (digit-prefix table).
