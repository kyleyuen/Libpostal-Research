# Experiment 073 — profile of the current head (diagnostic)

Status: complete (diagnostic; no candidate). Builds from 072 sources + timers (`sources/`: 033's context-fill markers ported, plus component-segment split ss_t[13]/[14]; `EXP_FTT_STATS` counters in trie_search.c). Harness 043 `bench_split.c` (and `tools/bench_split2.c` printing ss_t[13..14]); input 025 development.tsv (a first attempt passed the .txt file and exited 2; preserved in `raw/failed_attempt1/`).

- `raw/split_analyze.stdout`: pl vs dg (stage + cf timers), 3 alternating runs: dg changes context fill by −0.009 µs cold → 072's gain is layout noise.
- `raw/split_dgcf2_r*.stdout`: context-fill sub-stages (cold/warm µs): normalize 0.23/0.16, address dictionary 0.27/0.07, prefix/suffix 0.22/0.11, component phrases 1.04/0.12, postal codes 0.46/0.07.
- `raw/split_dgcf3_r*.stdout`: component segment = slot loop 0.34/0.04 + trie search 0.66/0.07.
- `raw/fst_dev.stderr`, `raw/fst_osm.stderr`: per search, tokens assisted by the first-token table, without entry, not fresh; assisted ends (fell off / leaf / interior); terminal and continuation reads.
These led to 074.
