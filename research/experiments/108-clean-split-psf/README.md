# Experiment 108 — cold/warm stage split of psf on the cleaned machine (diagnostic)

Date/time/timezone: created 2026-10-04T00:04:26-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: complete

105's stats build `pss` (psf + EXP_STAGE_STATS + postcode timer), 3 runs each on development (025 TSV) and OSM, 3 passes (`raw/split_analyze.stdout`). Development: total 5.828 / 2.526 µs cold / warm, so 3.30 µs (57%) is first-touch. Context fill 1.720 / 0.375; features 1.349 / 0.722; lookups 0.801 / 0.255; accumulate 0.969 / 0.463; Viterbi and rest 0.460 / 0.416. OSM: 3.937 / 2.234. For 7.8× on golden-v1 (31.29 / 7.8 = 4.01 µs vs psf 5.44 µs in 107), about 26% (1.4 µs per address) would have to go.
