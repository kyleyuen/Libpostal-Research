# Experiment 126 — 1–5-digit postcode-trie table plus two-stage prefetch of the walk's end (EXP_PDT 5 digits + EXP_PDT_PF)

Date/time/timezone: created 2026-10-04T02:27:39-0700 (`raw/created.txt`); screen 2026-10-04T02:30:05-0700 → 2026-10-04T02:35:59-0700; America/Los_Angeles. Status: **rejected (golden-v1 only; OSM slower)**

Change (`raw/address_parser.diff`, `raw/trie_search.diff`):
- **Table.** 111's exact table, extended to 1–5 digits (111,110 entries, 0.87 MB).
- **Prefetch, stage 1.** Right after token normalization, each digit-start admin token's table entry gives the walk's final node, which is prefetched.
- **Prefetch, stage 2.** Before the component search, that node is read and its terminal and continuation cells (or the leaf's data node) are prefetched.
- These follow the 074/077 pattern: issued a whole stage ahead, from data already in hand.
Correctness: `pdv` (with the entry recompute check) and `pdp` identical on golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2.
Screen vs cpi (`raw/screen_analyze2.stdout`):
        v1 median cpi 5.235 pdp 5.173; pdp speedup per round [1.0047, 1.0154, 1.0141, 1.0152, 1.0097, 1.0206] median 1.0147; rounds faster 6/6
        v1 min    cpi 5.136 pdp 5.075; pdp speedup per round [1.0025, 1.0206, 1.0166, 1.0016, 1.0102, 1.0258] median 1.0134; rounds faster 6/6
        osm median cpi 3.361 pdp 3.389; pdp speedup per round [0.9944, 0.9497, 0.9838, 1.0012, 1.0, 0.995] median 0.9947; rounds faster 1/6
        osm min    cpi 3.269 pdp 3.31; pdp speedup per round [0.9903, 0.9825, 0.978, 0.9967, 1.0266, 0.9906] median 0.9905; rounds faster 1/6
Decision: +1.5% on golden-v1 (6/6), but −0.5% on OSM (1/6 faster). The gain follows golden-v1's many 5-digit postcodes and does not generalize to OSM-style data, so it is rejected without spending an unseen set.
