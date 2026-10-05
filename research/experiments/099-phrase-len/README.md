# Experiment 099 — phrase strings joined with known word lengths (EXP_PHRASE_LEN)

Date/time/timezone: created 2026-10-03T22:02:46-0700; screen 22:05:19 → 22:10:30 (`raw/screen/start.txt`, `DONE`), America/Los_Angeles. Status: **inconclusive — real mechanism, net gain below the noise floor; not carried forward**

## Hypothesis and change
The 095 warm profile attributes ≈325 top-of-stack samples to `cstring_array_get_phrase`, plus ≈220 more to its `strlen`/`memmove` (≈3.5% of warm time). Most of these come from the 067 phrase memo and `word_or_phrase_at_index`. That function `strlen`s every word and appends the " " separator through another `strlen` + `memcpy`.
Change (`sources/address_parser.c`, `raw/address_parser.diff`): `ap_get_phrase` replaces the 7 calls in address_parser.c (all on the context's normalized / normalized_admin arrays). It takes each word's length from the cstring index (`cstring_array_token_length`) and pushes the separator as one byte. The words are normalized tokens of a NUL-terminated input and contain no NUL, so both lengths agree. `EXP_PHRASE_LEN_VERIFY` checks every word.
Base: npr (098); switches `raw/switches_pln.txt` = npr + `-DEXP_PHRASE_LEN` (verify build `plv` adds `-DEXP_PHRASE_LEN_VERIFY`).

## Correctness
`plv` and `pln`: golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2 are **all identical** (`raw/verify_*/summary.txt`). `plv` checked ≈16.8M words per fuzz seed, plus all corpus words, with no abort (`raw/verify_plv/*.err`).

## Screen (vs npr; 6 rounds, 20 passes; `raw/screen_analyze.stdout`)
    v1 npr [6.323, 6.303, 6.308, 6.325, 6.309, 6.298] median 6.309
    v1 pln [6.279, 6.296, 6.303, 6.308, 6.304, 6.302] median 6.303
    v1 pln speedup vs npr per round [1.007, 1.0011, 1.0008, 1.0027, 1.0008, 0.9994] median 1.001
    osm npr [4.014, 4.081, 4.04, 4.038, 4.03, 4.032] median 4.035
    osm pln [4.008, 4.029, 4.01, 4.013, 4.018, 3.998] median 4.011
    osm pln speedup vs npr per round [1.0015, 1.0129, 1.0075, 1.0062, 1.003, 1.0085] median 1.0069

## Mechanism (stage timers; `nss` = npr, `pss` = pln, both from this tree with EXP_STAGE_STATS; `raw/split_analyze.stdout`)

| µs per address, cold / warm | dev npr | dev pln | OSM npr | OSM pln |
|---|---:|---:|---:|---:|
| features | 1.563 / 0.815 | 1.527 / 0.793 | 1.027 / 0.662 | 1.010 / 0.646 |
| total parse | 6.664 / 2.854 | 6.601 / 2.859 | 4.351 / 2.461 | 4.369 / 2.472 |

The features stage is 0.02–0.04 µs faster, as intended. Total parse time does not move consistently, because other stages shift by a similar amount (layout).

## Decision
Inconclusive: an exact change with a real but tiny saving (≈0.3–0.5%), below the 0.4–0.5% build-to-build noise. It does not justify an unseen set on its own and is not stacked. Idea C's other sites are smaller still (095: no remaining strlen/memmove site above ≈1% of warm time), so C is closed unless bundled into the integer front half (idea F).
