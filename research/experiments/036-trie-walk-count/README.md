# Experiment 036 — trie node transitions per address in phrase search (measurement only)

Date/time/timezone: 2026-10-02 16:13–16:17 PDT (`raw/created.txt`, `raw/run.log`).
Status: complete

Counting build `wc` = `l3` sources + switch `EXP_WALK_COUNT` in `trie_search.c` (per trie: calls, node transitions, terminal checks, tail compares, partial-match restarts, phrases found; later also first-token walks). -O2, no PGO/LTO (counts only). `dump_outputs` on golden-v1 development and OSM. Attempt 1 crashed at exit (destructor read a freed trie; outputs truncated) — preserved in `raw/attempt1_crash/`, fixed by recording node counts at first use. Final outputs complete (10,308 / 11,405 lines) and identical between the two counting builds.

Per address, development split (`raw/walk_v1.txt`, `raw/walk3_v1.txt`): address dictionary (253k nodes) 20.2 transitions, component phrases (8.2M nodes) 33.7, postal codes (7.5M nodes) 19.4. First-token walks from the root account for 91% / 77% / 88% of transitions. Combined with 033's times this is ≈12 ns per transition for the small trie and ≈21–27 ns for the large ones (a time/count ratio, not a cache-miss measurement). OSM: similar (`raw/walk_osm.txt`). Used to design 037 (rejected).
