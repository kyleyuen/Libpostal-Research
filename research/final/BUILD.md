# Final candidate `h` — how it is built

The sources in `sources/` are the original libpostal files with the accepted changes, guarded by `EXP_*` macros. The candidate is the original static library `libpostal.a`
(built from the untouched tree in `libpostal/`) with these objects replaced. Compile each file exactly like the library does (`gcc -std=gnu23 -DHAVE_CONFIG_H ... -g -O2 -D LIBPOSTAL_EXPORTS`,
see `experiments/011-normalize-fastpaths/tools/build.sh` for the verbatim command) with these extra flags, run from `libpostal/src` so its headers resolve, with `sources/` copied next to each compiled file:

| Object | Source file(s) | Extra flags |
|---|---|---|
| `libpostal_la-crf.o` | `crf.c`, `crf.h`, `feature_dict.h`, `trie.h` | `-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT` |
| `libpostal_la-trie.o` | `trie.c`, `trie.h` | none |
| `libpostal_la-string_utils.o` | `string_utils.c` | `-DEXP_FAST_APPEND` |
| `libpostal_la-features.o` | `features.c` | `-DEXP_FAST_JOIN` |
| `libpostal_la-address_parser.o` | `address_parser.c`, `feature_dict.h` | `-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE` |

Then `ar -r libpostal.a` those five objects into a copy of the baseline archive. `patches/*.patch` are unified diffs of each file against the original source; `feature_dict.h` is new.
Experiment-only environment switches remain in the code (`CRF_DICT_DISABLE`, `CRF_DICT_VERBOSE`, `FD_VERBOSE`); a production patch should remove them.

## What each change does (all exact, single-threaded)
crf.c/crf.h: batch all lookups per address; no redundant context reset; lean Viterbi context; direct key lookups with weight-row prefetch; load-time conversion of the two feature tries into `feature_dict.h` dictionaries (old tries freed; original trie kept on any doubt).
trie.c/trie.h: known-length lookup helper (used by the pre-dictionary path). string_utils.c: memcpy-based append. features.c: fast feature-string join. address_parser.c: no per-token clearing while the CRF accumulates,
vocabulary trie converted to a dictionary, exact ASCII fast path for whole-string normalization, shared admin token normalization.
See experiments 003, 005, 006, 009, 010, 011 for each change, its measurements and its correctness evidence; 004 and 012 are rejected attempts.
