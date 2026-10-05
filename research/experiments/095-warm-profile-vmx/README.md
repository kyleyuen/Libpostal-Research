# Experiment 095 — warm (compute-only) function profile of vmx (diagnostic)

Date/time/timezone: created 2026-10-03T11:32:10-0700, America/Los_Angeles. Status: complete

Method as 089 (vmx bench_ext cycling 300 development addresses, `sample` 20 s; `raw/sample.txt`, 15,457 top-of-stack samples). Top: address_parser_parse (inlined) 4,886; crf_dict_lookup_array 1,384; trie_search_tokens_from_index 1,292; address_parser_features 933; memmove 866; memset 857 (854 inside parse: score-matrix clearing); fd_find_slot 663; strlen 553; feature_array_add_nb 541; normalize_token 485; ap_ik_emit1 389; scan_token 327.
Note: folding the score-matrix clearing into the Viterbi loop is incompatible with 091 (the backtrack re-reads the scores). Next target: dictionary lookups (≈9%) → 096.
