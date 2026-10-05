# Experiment 104 — coverage and exactness probe for a "falls off the postcode trie" flag (diagnostic)

Date/time/timezone: created 2026-10-03T23:00:02-0700 (`raw/created.txt`); commands with timestamps in `raw/commands.txt`. America/Los_Angeles. Status: complete (diagnostic; no timing)

## Question
Can the postcode-phrase search (073: 0.46 µs cold / 0.07 µs warm per address, measured on an older head) skip the walks of tokens that cannot start a postcode, using a flag derived from the model and stored in a spare bit of the 056 first-token-table entry? The postcode search walks the same admin tokens whose entries the slot loop already fetches and 074 prefetches, so the flag adds no memory and no new cold table.
This is 016 T6 / 019's `EXP_PHRASE_PREFILTER` idea, which was exact but was rejected in 020 (−2.5%) because its separate byte/pair-table probes cost more than the walks they skipped.

## Build
`pcf` / `pcf2` = psf (101) sources + `EXP_PCF_PROBE`, no PGO/LTO (`raw/commands.txt`). `pcf2` adds a fall-depth histogram.
- **Load.** `trie_word_falls_off` (`sources/trie_search.c`) walks every vocabulary word from the postcode trie's root with the search loop's own transitions and fall-off test (`node.check != last_node_id` with `last_node.base >= 0`). A tail or the end of the word counts as "no". The result goes in bit 60 of the word's FTT entry.
- **Search.** For every token in a fresh state (the same condition as `EXP_PHRASE_PREFILTER`, start node = root), the counters record:
  - whether the original walk actually fell off;
  - whether the token had an admin slot and the flag set;
  - any flagged token that did not fall off. That case would make the flag inexact.

## Results (`raw/probe_*.stderr`, `raw/probe2_*.stderr`)
Vocabulary words whose walk falls off the postcode trie: 2,847,397 of 3,694,560.

| set | searches | fresh tokens | fresh, fell off | fresh, with slot | flagged (skippable) | flagged and fell off | **flagged, did NOT fall off** |
|---|---:|---:|---:|---:|---:|---:|---:|
| development | 10,308 | 60,172 | 26,641 | 39,279 | 24,252 (40% of fresh) | 24,252 | **0** |
| OSM | 11,405 | 60,380 | 28,749 | 40,135 | 27,362 (45%) | 27,362 | **0** |
| golden-v1 all | 12,863 | 75,149 | 33,334 | 49,057 | 30,326 (40%) | 30,326 | **0** |
| fuzz seed 1 (2M) | 1,788,056 | 13,555,198 | 6,384,123 | 7,900,088 | 4,766,474 | 4,766,474 | **0** |

Flagged tokens cover 91% (dev) / 95% (OSM) of fresh walks that fall off. The rest of the fresh tokens walk into the trie, mostly numeric tokens, which have no vocabulary slot.
Fall position of the original walk, development (OSM): flagged walks fall at byte 0/1/2/3/4+ = 1,081 / 6,848 / 13,449 / 2,531 / 343 (1,398 / 7,013 / 13,809 / 4,659 / 483). Most flagged walks read 2–3 trie nodes beyond the root, so a skip avoids real node reads rather than only the root's row.
Outputs of the probe build: OSM and fuzz seed 1 are identical to the original (counters only).

## Interpretation
The flag is exact on every input seen, including 4.8M flagged fuzz tokens, and covers about 2.4 tokens per address (dev 2.35, OSM 2.40). The speed effect is **not measured**. The skipped work is 1–3 node reads per flagged token, against a postcode stage of 0.46 µs cold on an older head. Any estimate of the gain would be an unverified assumption. The next step would be candidate 105:
- set the flag in `ap_ftt_build`;
- keep the admin slots alive until the postcode search;
- skip flagged fresh tokens exactly as `EXP_PHRASE_PREFILTER` does, with a `_VERIFY` mode that runs both paths;
- screen against psf, check the mechanism with stage timers, then golden-v9.
Load cost of the extra pass: not timed separately (it shares the 101 vocabulary loop).
