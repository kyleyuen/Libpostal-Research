# Experiment 039 — feasibility count: one-lookup table of vocabulary-word walks in the phrase tries (model-only measurement)

Date/time/timezone: created 2026-10-02 (see `raw/created.txt`), run 16:53–16:58 PDT (`raw/run.log`).
Status: complete — idea **not pursued** (does not fit the memory margin for the expected gain)

## Question
038 showed the component/postal phrase searches are dominated by first-touch memory cost. Could a model-derived table give each vocabulary word's walk from the trie root in one lookup (keyed by the vocabulary slot id the parser already computes), replacing ~6 scattered node reads per token, within `l3`'s ~31 MB steady-memory margin?

## Method
Diagnostic switch `EXP_WORDWALK_COUNT` (experiment copy of `l3` sources): on the first parse call, every vocabulary key (3,694,560) is walked from the root of the component-phrase and postal-code tries with the token search's per-byte rules, and the outcome (falls off / reaches a leaf tail / consumes the whole word) is binned by the number of steps. Model data only; no address corpus. First attempt crashed (exit 139) because the count ran at load before the tries existed; moved to the first parse (`raw/run.log`). Output `raw/ww_count.stderr`.

## Results
Component trie (8,220,951 nodes): falls off after 1 / 2 / 3–4 / ≥5 steps: 10,467 / 5,237 / 123,081 / 1,976,001 words; reaches a leaf tail: 54 / 742 / 68,557 / 1,210,720; consumes the whole word inside the trie: 43 / 819 / 41,428 / 257,411 (158,460 of these end where a key terminates, 132,956 where a space continuation exists).
Postal-code trie (7,498,166 nodes): vocabulary words mostly fall off within 2–4 steps (719,232 + 2,044,430); only 481 consume the whole word. The expensive postal walks in real addresses are numeric tokens, which are not vocabulary words, so a vocabulary-keyed table does not help there.

## Decision
A table that covers the walks that matter (whole-word and leaf outcomes of ≥3 steps, ≈1.58M words) needs roughly 30–50 MB at 8–12 bytes per entry plus hashing overhead, exceeding `l3`'s steady margin; funding it by returning to denser dictionaries (`m3`, ≈−1.5% speed, ≈+115 MB) leaves an estimated net gain of only ~4–5% (UNVERIFIED estimate from 033/038: component search cold cost ≈1.0 µs, of which perhaps half removable). Not pursued.
