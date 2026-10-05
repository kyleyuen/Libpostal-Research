# Experiment 065 — warm-only profile of 062 `lf` (diagnostic)

`sample` of `lf` cycling the first 30 golden-v1 development addresses (`raw/`). Top exclusive: parse (inlined) 36.3%, feature_array_add_nb 6.2%, memset 6.2% (per-address score-buffer clearing, inlined into parse), crf_dict_lookup_array 6.2%, trie search 5.6%, memmove 5.2% (cstring_array_add_string 1.6, normalize_token 0.74, cstring_array_get_phrase 0.61, …), address_parser_features 4.7%, strlen 3.9% (feature_array_add_nb 1.53), normalize_token 3.5%, ap_ik_emit1 2.9%, fd_find_slot 2.3%. Basis for 066.
