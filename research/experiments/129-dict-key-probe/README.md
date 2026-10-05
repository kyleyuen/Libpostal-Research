# Experiment 129 — address-dictionary trie keys per data id; runtime phrase vs key (diagnostic)

Date/time/timezone: created 2026-10-04T02:49:54-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete

Build `dk` = cpi + `EXP_DK_PROBE`, no PGO; `raw/probe_*.stderr`.
- **Load.** 153,670 keys enumerated from the root, all checked with `trie_get_data`; 78,822 carry a '|' or a prefix/suffix marker; every data id has exactly one key.
- **Runtime.** Dictionary phrase string equals the key: development 10,945 equal / 18 different; OSM 9,914 / 16; fuzz 200k 250,186 / 458. Every difference is a key matched across tokens without a space (e.g. runtime `大 街` vs key `大街`), which the length guard rejects, as in 118.
→ 130.
