# Experiment 123 — string keys per address remaining after cpi (diagnostic)

Date/time/timezone: created 2026-10-04T02:22:29-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete

117's probe (`EXP_SK_PROBE`) ported to cpi sources (`sk`, no PGO); `raw/probe_*.stderr`. String keys per address: development 17.5 (was about 31 before 119), OSM 14.1.
Remaining, development: `prev word+word` 4.80, `word+next word` 4.76, `word` 1.79, `phrase type+phrase` 1.28 (address-dictionary phrases), `postcode no context` 1.06, `postcode have context` 0.71, `unambiguous phrase type(+phrase)` 0.52 each, next/prev word 0.50/0.41, trans word/prev word 0.41/0.35.
The bigrams are about 55% of what remains; 122 showed the integer-table route does not pay for them.
