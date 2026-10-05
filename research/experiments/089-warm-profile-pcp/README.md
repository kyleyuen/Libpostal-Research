# Experiment 089 — warm (compute-only) function profile of pcp (diagnostic)

Date/time/timezone: created 2026-10-03T04:55:40-0700, America/Los_Angeles. Status: complete — **no exact change above the 2–3% bar left**

Method (049's, larger set): pcp `bench_ext` cycling the first 300 development addresses (`raw/dev300.tsv`, data stays cached), `sample <pid> 15 1` after 25 s (`raw/sample.txt`; ≈11566 top-of-stack samples).
Top of stack: address_parser_parse (LTO-inlined context fill, accumulation, Viterbi) 3,875; trie_search_tokens_from_index 1,016; crf_dict_lookup_array 946; memmove 664; address_parser_features 652; memset 613; fd_find_slot 485; strlen 417; feature_array_add_nb 401; normalize_token 297; ap_ik_emit1 291; feature_array_add_nbl 241; scan_token 214; mach_absolute_time 156 (harness timing).
Callers: memset — 610 of 613 inside address_parser_parse (per-address clearing of the state / state-transition score matrices; exact alternatives were tried in 016 T8 and 066 and lost); strlen — spread over feature_array_add_nb 105, parse 93, features 56, cstring_array_get_phrase 48, cstring_array_add_string 41, ap_ik_emit1 33 …; memmove — cstring_array_add_string 161, cstring_array_get_phrase 124, parse 103, normalize_token 64 ….

Conclusion: the only libc overheads above 3% (memmove ≈6%, memset ≈5.6%, strlen ≈3.8% of warm time) are either already-tested (memset) or spread across many string-building sites, so an exact change would save roughly 1% of total time each — at the timing noise floor. The large items (phrase search, dictionary lookup, the inlined parse body) are the algorithm itself. Together with 088, further gains need the front-half rewrite, not more local changes. No further experiments opened.
