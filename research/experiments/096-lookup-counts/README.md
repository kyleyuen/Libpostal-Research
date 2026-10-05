# Experiment 096 — what the dictionary-lookup stage does per address (measurement)

Date/time/timezone: created 2026-10-03T11:33:57-0700, America/Los_Angeles. Status: complete (diagnostic)

Counter build `EXP_LK_STATS` on vmx sources (-O2; `raw/lk_*.stderr`):
    development LK_STATS state per address: string keys 30.61 (name-byte 28.76, split 1.85, mean 13.5 bytes), int keys resolved 15.26, int keys via table 0.00, found 38.59
    development LK_STATS trans per address: string keys 0.76 (name-byte 0.76, split 0.00, mean 14.1 bytes), int keys resolved 7.98, int keys via table 0.00, found 8.33
    OSM LK_STATS state per address: string keys 26.02 (name-byte 25.18, split 0.84, mean 13.4 bytes), int keys resolved 14.18, int keys via table 0.00, found 35.94
    OSM LK_STATS trans per address: string keys 0.48 (name-byte 0.48, split 0.00, mean 13.7 bytes), int keys resolved 7.28, int keys via table 0.00, found 7.53
Every integer key is already resolved to a feature id when it is produced (no integer-table lookups remain); the lookup stage hashes and compares ≈26–31 string keys per address (≈13.5 bytes, mostly 1-byte-name form), ≈9 ns each warm. No cheap exact reduction identified (the value hash is seeded per name, so keys sharing a value cannot share hashing without a new table hash; savings estimated well below 1%).
