# Experiment 117 — which feature families still reach the lookup as string keys (diagnostic)

Date/time/timezone: created 2026-10-04T01:03:59-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: complete

Build `sk` = rt8 + `EXP_SK_PROBE` (`sources/crf.c`), no PGO. Every string key decoded in `crf_dict_lookup_array` is counted by feature name, with mean value length. A first run read the name table after the dictionary was freed and printed garbage names; it is kept in `raw/attempt1_names_freed/`.
Results per address (`raw/probe_dev.stderr`, `raw/probe_osm.stderr`), development (OSM):
- state `phrase type+phrase` 6.06 (5.95), `prev word+word` 4.80 (4.32), `word+next word` 4.76 (4.15), `phrase` 3.12 (2.75), `commonly city` 2.19 (2.03), `unambiguous phrase type+phrase` 1.95 (1.40), `unambiguous phrase type` 1.95 (1.40), `word` 1.79 (0.83), `postcode no context` 1.06 (1.08), `postcode have context` 0.71 (0.68), next/prev word 0.5/0.4.
- trans: `word` 0.41, `prev word` 0.35.
Interpretation: phrase-keyed families (about 11/address) and word bigrams (about 9.6/address) dominate the remaining string keys. Integer keys for them are idea F's largest pieces; the estimated gain is a few percent (UNVERIFIED).
