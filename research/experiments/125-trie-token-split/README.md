# Experiment 125 — time per trie-search token iteration, by trie and token class (diagnostic)

Date/time/timezone: created 2026-10-04T02:25:07-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete

Build `tt` = cpi + EXP_STAGE_STATS + `EXP_TT_PROBE` (`sources/trie_search.c`: per-iteration timer by trie and class). Harness `tools/bench_split_tt.c` (043's paired cold/warm harness, printing the classes); 3 runs each (`raw/split_*`). Run 2, cold / warm µs per address:

| trie / class | development | OSM |
|---|---:|---:|
| component, fresh digit-start | 0.036 / 0.018 | 0.036 / 0.017 |
| component, fresh other | 0.222 / 0.025 | 0.063 / 0.021 |
| component, inside walk | 0.186 / 0.025 | 0.071 / 0.025 |
| **postcode, fresh digit-start** | **0.244 / 0.022** | 0.076 / 0.019 |
| postcode, fresh other | 0.077 / 0.030 | 0.067 / 0.027 |
| address dictionary (other), fresh other | 0.175 / 0.055 | 0.119 / 0.051 |

The postcode stage's first-touch cost on golden-v1-style data is in digit-start tokens. 111's 1–4-digit table removed only the near-root steps; the rest is the deep end of 5-digit walks (final node, terminal and continuation cells, data node). → 126.
