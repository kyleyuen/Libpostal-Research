# Experiment 118 — can component-phrase features be keyed by the trie's data id? (diagnostic)

Date/time/timezone: created 2026-10-04T01:18:42-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: complete

Build `ck` = rt8 + `EXP_CK_PROBE` (`sources/address_parser.c`), no PGO. At load it enumerates the component-phrase trie depth-first over the double array, checks every key with `trie_get_data`, and keeps data id → key. At parse time it compares the runtime component phrase string (admin tokens joined by spaces) with the key for its data id.
Load (`raw/probe_dev.stderr`): trie num_keys 4,012,750; enumerated 4,012,750; 0 failed the check; every data id has exactly one key; mean key 14.7 bytes.
Runtime (`raw/probe_*.stderr`): development 32,103 equal / 0 different; OSM 31,345 / 0; golden-v1 all 40,132 / **2 different**; fuzz seed 1 (200k inputs) **several different**.
Examples: runtime `ზღვის უბანი` vs key `ზღვისუბანი`; `aespa alev ik` vs `aespa alevik`. The trie match can join tokens without a space.
Consequence: features precomputed per data id need a runtime guard. The match consumes the token bytes in order with either "" or " " between tokens, so equal lengths imply equal strings (to be verified on every use by a verify mode). → 119.
