# Experiment 127 — warm function profile of cpi on OSM addresses (diagnostic)

Date/time/timezone: created 2026-10-04T02:37:02-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete

cpi `bench_ext` cycling the first 300 OSM addresses (`raw/osm300.tsv`), `sample` for 20 s (`raw/sample.txt`; 15,471 samples; warm 2.50 µs/address). Top functions (`raw/top.txt`):
- `address_parser_parse` (inlined) 37.4%
- `trie_search_tokens_from_index` 8.2%
- **`memset` 7.4% (0.19 µs)**: almost all of it is the score-matrix clearing at the start of each address
- `crf_dict_lookup_array` 5.9%; `address_parser_features` 5.7%; `memmove` 5.5%; `fd_find_slot` 4.2%; `feature_array_add_nb` 3.8%; `strlen` 3.1%
→ 128.
