# Experiment 124 — cold-minus-warm line profile of cpi (diagnostic)

Date/time/timezone: created 2026-10-04T02:23:26-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete

Method as 112, line level (`tools/pc_profile.py`, xctrace CPU Profiler, `atos -i`). Output: `raw/pc_profile_inline.stdout`. Top cold − warm (xctrace scale; cold 5.18 / warm 3.46 µs):
- trie_search.c line 0 + :546 + trie.c ≈ 0.28: dependent trie-node reads in the phrase searches;
- crf.c :693/:671/:689/:707 ≈ 0.26: accumulation;
- `fd_next` 0.09;
- vocabulary `fd_find_slot` ≈ 0.11;
- `pc_has_edge` 0.045.
→ 125.
