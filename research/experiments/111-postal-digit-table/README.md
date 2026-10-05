# Experiment 111 — digit-prefix table for the postcode trie (EXP_PDT)

Date/time/timezone: created 2026-10-04T00:12:18-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: **rejected (no effect)**

Change (`raw/trie_search.diff`, `raw/address_parser.diff`):
- **Load.** Every ASCII digit string of 1–4 bytes (11,110 strings; 89 KB static table) is walked in the postcode trie with 056's `trie_ftt_walk`; all 11,110 entries are representable.
- **Search.** A fresh-state token starting with ASCII digits uses the entry of its first min(m, 4) digits to start the byte loop at that entry's last step, which is exactly 056's mechanism. Motivation: 109 showed digit tokens walk 4–5 steps.
Correctness: `pdv` (+ `EXP_PDT_VERIFY`: every entry used is recomputed, and every postcode search is re-run without the table and compared; + 056/101 verify modes) and `pdt`: all identical on golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2 (`raw/verify_*`).
Screen (`raw/screen_analyze2.stdout`; noisy, load average 4.7 from interactive apps):
        v1 median psf 5.737 pdt 5.823; pdt speedup per round [0.9371, 1.014, 1.0041, 1.0044, 0.9771, 0.9959] median 1.0; rounds faster 3/6
        v1 min    psf 5.596 pdt 5.578; pdt speedup per round [0.9839, 1.0125, 1.0068, 1.0345, 0.9833, 0.9958] median 1.0013; rounds faster 3/6
        osm median psf 3.74 pdt 3.699; pdt speedup per round [1.0075, 0.9898, 1.0371, 1.0252, 1.0314, 0.9673] median 1.0164; rounds faster 4/6
        osm min    psf 3.579 pdt 3.552; pdt speedup per round [0.99, 0.9848, 1.0035, 1.0335, 1.0426, 0.9535] median 0.9968; rounds faster 3/6
Stage timers (`raw/split_analyze.stdout`; `pss` = psf, `dss` = pdt, 4 alternating runs): postcode search, dev 0.412 / 0.058 → 0.414 / 0.059 µs; OSM 0.214 / 0.054 → 0.213 / 0.056. **No change.**
Conclusion: the deep digit walks do not cause the postcode search's first-touch cost (≈0.35 µs dev). Together with 105 (fall-off walks), neither the shallow nor the deep transitions of fresh tokens are it. The remaining candidates are the terminal and continuation cells, the tails and data nodes of tokens that walk in, and the multi-token continuation (unverified). Not pursued further for now.
