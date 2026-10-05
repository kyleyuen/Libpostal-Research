# Code audit: where the remaining time can still be removed (audit only, nothing here was run)

> Historical audit of the older `h` build. For the final `l4` build and experiment 025, read the [2026-10-01 consolidated audit](FINAL_AUDIT_2026-10-01.md). In particular, the non-ASCII subset's measured 2.33× result below is not a guaranteed lower bound on unseen data, and optimization estimates below are not measured gains in `l4`.

Date: 2026-09-30. Scope: every function on libpostal's address-parse path, read in the source tree under `libpostal/src` and cross-checked against the profiles taken of the baseline and of the current best build `h` (`experiments/008-profile-after-006/raw/`).
For each function the audit asks the two questions you set: **(1) is this necessary? (2) can it be made faster, and how?** Nothing in this document was compiled or timed; every gain below is an **estimate** (basis given) and is labelled UNVERIFIED until an experiment measures it.
Constraints unchanged: byte-identical output, one thread, no extra memory, nothing derived from benchmark inputs (model-derived tables and code-derived proofs only).

## 1. Direct answers

**A. "What is the definite speedup on a random dataset?"** (measured in experiment 013, real unmodified addresses only, whole-set numbers, no synthetic data)

| Input set | Speedup of the current best build |
|---|---:|
| the full real set (12,863 addresses, 88 countries) | **2.64x** |
| eight unbiased random samples of 2,000 addresses of that set | 2.67x to 2.71x (median 2.69x) |
| **floor: only the 5,404 real addresses that get no benefit from the ASCII normalization path** (non-ASCII, `&` or control characters) | **2.33x** |
| the 7,459 ASCII addresses | 2.91x |

A speedup on data nobody has seen cannot be "definite", but it is bounded: no input in any set was slower, the measured floor is **2.33x**, and if the unseen set looks like the public data the expected figure is about **2.6x to 2.7x**. Use the full-set number (2.64x) as the headline and the floor (2.33x) as the guarantee.

**B. "You cheated in experiment 011."** I want to be exact about what was and was not done. The ASCII fast path was derived from reading the code and enumerating the transliterator's rule tables (every rule starts with `&` or with a non-ASCII byte), not from the benchmark data, and it changes no output (byte-identical on the full set, on 4,000,000 fuzzed inputs, and on 80 million differential cases).
What I did do wrong is **lead with a number that depends on the ASCII share of the data** (58% of the public set is ASCII-eligible) and report the synthetic "novel" set beside it as if it were independent evidence. The synthetic set is built from the same public records and is 60% ASCII, so it tells you nothing new; I should have led with the floor, and I now do. The same audit also shows the dependence is a fixable weakness, not something to hide: the non-ASCII half of the data still pays about 3.3 µs per address for normalization that can be made exact and fast (items N1 to N7), which would remove the dependence on ASCII share altogether.

**C. "Do you really think it is only 2.something?"** No, not as a ceiling. My earlier estimate of "about 3x at best" was based on micro-optimizations of things I had already profiled. Reading the code function by function finds far more than I had listed: **66 items in section 3** (60 numbered items plus 6 additional ones; three numbered entries are cross-references, so 63 are distinct), several of which are structural (a different data layout, memoization across tokens, exact prefilters in front of cold tables) and were not on my earlier list.
The arithmetic (section 5): 3.5x on the full set means 9.97 µs per address, against 13.22 µs now, a further 3.25 µs (25%). After discounting 30% for overlap between items, the estimates in section 3 total 4.5 µs to 7.7 µs (section 5). Because my estimates in this project have realized anywhere from negative (experiment 012 lost 3 for 3) to three times the estimate (the vocabulary dictionary), I weight them by a realization factor of 0.35 to 1.0, central 0.6. That gives an expected result of **about 3.0x to 4.0x**, and I put the chance of reaching 3.5x at roughly even odds (about 50-55%), with most of the gain coming from items that change *structure* (C1, N1 to N6, F1 to F3, R1 to R3, P1). That is a judgment, not a measurement.

## 2. How the audit was done and what the time is spent on

Files read in full or in the relevant functions: `libpostal.c` (parse entry, response destroy, setup), `address_parser.c` (parse, context fill, feature function, phrase helpers, response build), `tokens.c`, `scanner.re` (tokenize loop), `normalize.c`, `transliterate.c` (engine), `string_utils.c` (arrays, `utf8_case`), `trie.c`, `trie_search.c` (all three searches), `address_dictionary.c` (search wrappers), `graph.c`, `features.c`, `ngrams.c`, `crf.c`, `crf_context.c` (Viterbi), `matrix.h`/`vector.h` (resize and growth), `log/log.h` (debug macros compile to no-ops at the default level, verified).
The 78 functions that reach 0.15% of samples in the baseline profile are each covered in section 4.

Where the current best build (13.22 µs per address on the full set) spends its time, from the profile of build `h` (shares are of samples and approximate, from one 15 s profile; inclusive shares overlap):

| Stage | Share | About µs |
|---|---:|---:|
| `context_fill` (token normalization, three phrase searches, prefix/suffix searches) | 24% | 3.2 |
| feature generation (`address_parser_features`, `feature_array_add`, vocab lookups) | 19% | 2.5 |
| dictionary lookups (`crf_dict_lookup_array`, `fd_split`, key compare) | 12-15% | 1.6-2.0 |
| weight accumulation and per-address zeroing (`crf_tagger_score` own time) | 8.7% | 1.15 |
| Viterbi | 5.2% | 0.7 |
| allocation and freeing | 5.7% | 0.75 |
| whole-string normalization (mostly the non-ASCII half) | ~10% | ~1.3 |
| tokenizing | 2.4% | 0.32 |
| response building and freeing | ~3% | ~0.4 |
Measured cross-check: non-ASCII addresses cost 14.67 µs against 11.38 µs for ASCII ones in the current build (experiment 013), so the non-ASCII normalization path still costs about 3.3 µs per such address.

Algorithm families used below, so you can see the space that was considered: **dynamic programming / memoization** (F1, F2, P7, N3), **divide and conquer** (N5, N6: split a string into context-free segments; P1: split the search by first-byte), **greedy / early exit** (P1, P2, N1, N2, F3), **niche data structures** (2-byte bitmaps and filters in front of cold tables, perfect and cuckoo-style hashing, Eytzinger layout, succinct rows, inline row layouts, bump-pointer arenas, sparse undo instead of clearing), **SIMD / SWAR** (N8, P4, X2), **layout transformations** (array-of-structs to struct-of-arrays, co-locating keys and weights), and **compiler-level** (X1 to X4).

## 3. The items

Format: **ID title** — *where* · *what it does now* · *necessary?* · *proposal (algorithm)* · *why exact* · *estimate (basis)* · *effort/risk*. "Now" figures are from the profiles; estimates are UNVERIFIED.

### Group N — input, normalization, tokenizing (about 10-12% of time now; the biggest structural weakness)

**N1 Skip the `html-escape` transliteration step when the string has no `&`.** *transliterate.c:729-, the steps loop over `latin-ascii-simple`'s two steps.* Step 1 has 65,788 rules and every one starts with `&` (enumerated in experiment 011). Today every non-ASCII string still runs the full character state machine for that step. *Necessary?* No, for any string without `&`. *Proposal (greedy early exit):* one `memchr('&')`; skip the step. *Exact:* the rule table proves no rule can start elsewhere. *Est.:* removes one of two engine passes for the 42% non-ASCII inputs: about 0.4 to 0.6 µs on those, 0.2 to 0.3 µs average (1.5-2%). *Effort:* small, low risk.

**N2 Skip the second transliteration step when no rule can start.** *transliterate.c same loop; ruleset `latin-ascii-simple|1`.* Its 138 rules start only with the UTF-8 lead bytes C2, C3, CB, E2, E3, EF. A table of the exact first two bytes of each rule (built at load from the trie with the enumerator that already exists) lets the step be skipped when no position matches. Common Latin text (accents are C3 xx) mostly does not match any of the few C3 rules. *Necessary?* Only when a rule can fire. *Exact:* table derived from the model's own rules. *Est.:* another 0.4 to 0.8 µs on non-ASCII inputs (about 0.2-0.3 µs average, 1.5-2%). *Effort:* small-medium; the engine has contexts and sets, so the skip test must be conservative (first codepoint of every rule, not just bytes).

**N3 Memoize per-call transliterator constants.** *transliterate.c:692-735 (`string_is_lower(trans_name)`, `get_transliterator(name)` hash lookup, three `trie_get_prefix*` walks: the transliterator name, the namespace separator, and each step name plus separator).* All are constants for `"latin-ascii-simple"`; they are recomputed on every call (this is the `trie_get_prefix_from_index` 1.2-1.4% in the profiles). *Necessary?* Once, at setup. *Proposal (DP/memoization):* cache the resolved `trie_prefix_result_t` per (transliterator, step) in the transliterator struct. *Exact:* pure functions of constants and the immutable trie. *Est.:* 1.2-1.4% of time (about 0.17 µs), mostly on non-ASCII inputs. *Effort:* small.

**N4 Remove the per-call copies in `transliterate`.** *transliterate.c:688 (`str = strndup(str, len)`), :761 (`char_array_new_size(len)` per step), and the final copy out.* Each call copies the input, allocates an output array per step and frees them. *Necessary?* No: work on the caller's buffer and two reusable output arrays. *Est.:* about 0.1 to 0.2 µs on non-ASCII inputs (allocator share 5.7% overall, part of it here). *Effort:* small.

**N5 Fuse the two Unicode passes into one.** *string_utils.c:228 `utf8_case` and normalize.c:24 `normalize_string_utf8_languages`.* `utf8_case` runs `utf8proc_decompose` twice (once to size the buffer, once to fill it), a per-codepoint `utf8proc_tolower` loop, `utf8proc_reencode` and a `realloc`; then `utf8proc_map` (utf8proc.c, verified) again runs `utf8proc_decompose` twice and `utf8proc_reencode` once with more options, plus a `malloc` and a `realloc`. That is four decompose passes, two reencode passes, a lowercase pass, two mallocs and two reallocs per string. *Necessary?* No; the result is one function of the input. *Proposal (divide and conquer):* split the string into runs of code points whose normalization is context-free and handle each run by table lookup, falling back to the library for anything with combining marks or composition contexts. *Exact:* verified per code point against the library over every Unicode scalar and over random strings of eligible code points, as done for ASCII. *Est.:* the remaining normalization cost of non-ASCII inputs (about 2 to 3 µs each, measured gap 3.3 µs) falls to about 0.3 µs: 1.0-1.3 µs average, **8-10%**. *Effort:* medium-high; this is the largest single structural item.

**N6 Per-code-point normalization table built at load.** *supports N5.* For each BMP code point (65,536 entries, built by calling the library's own normalizer on the one-code-point string at load, about 0.2 s) store the output bytes or "context-dependent, use the library". *Exact:* by construction for entries flagged context-free; the flag is verified by pair tests and random-string tests. *Est.:* included in N5. *Effort:* medium; memory about 0.5 MB.

**N7 Widen the ASCII fast path to all of ASCII.** *address_parser.c `address_parser_normalize_string`.* Today control characters and any `&` fall back. The measured table (experiment 011) says 0x01-0x08, 0x0e-0x1f and 0x7f are dropped, tab becomes a space, 0x0b-0x0d become newlines; `&` is safe unless followed by a byte pair that starts an entity rule. *Est.:* rare inputs, under 0.5%. *Effort:* small. Its value is removing the last "special cases fall back" corner, not speed.

**N8 SIMD for the ASCII checks.** *the eligibility scan and lowercase loop of the current fast path.* 16 bytes per step with NEON instead of a byte loop; also a NEON `memchr`-style scan for `|`, `&` and `-` used elsewhere. *Est.:* 0.03-0.06 µs (about 0.3%). *Effort:* small.

**N9 Return "unchanged" instead of a copy.** *address_parser.c:1672-1676; the parse already treats a NULL result as "use the original".* For an already-lowercase ASCII string the fast path could return NULL rather than allocating and copying an identical string (the later `strndup` in `tokenized_string_new_from_str_size` copies again anyway). *Est.:* 0.05 µs (0.4%). *Effort:* small.

**N10 Stop rebuilding the tokenized string per address.** *tokens.c:13-37 and address_parser.c:1678-1696.* `tokenize()` allocates a new `token_array`; `tokenized_string_new_from_str_size` then allocates a struct, a `cstring_array` sized to the string length, a `token_array` sized to the string length in *elements* (`token_array_new_size(len)`, 24 bytes each; the `num_tokens` argument is ignored), and `strndup`s the whole string again; every token is then copied into the `cstring_array` and pushed a second time into a second token array. The parse context already owns an unused `tokenized_str` (address_parser.c:618-620 region). *Necessary?* One tokenization into reused buffers is enough. *Proposal (arena):* tokenize straight into the context's arrays; take ownership of the normalized buffer instead of `strndup`. *Est.:* about 0.25 µs (2%), part of the allocation share. *Effort:* medium (ownership rules).

**N11 Remove dead per-address bookkeeping.** *address_parser.c:1682-1696 (`context->separators` clear/push/pop per token) and :1113 (`prev2_tag_features` cleared per token).* `separators` is read once into an unused local (line 1109); `prev2_tag_features` is filled only for the perceptron model (line 1477-1478) and is dead for the CRF. *Necessary?* No. *Exact:* no output depends on them. *Est.:* 0.05 µs (0.4%). *Effort:* trivial.

**N12 Exact ASCII specialization of `scan_token`.** *scanner.re:7 (re2c DFA, 310k generated lines); tokenizing is 2.4% inclusive.* Words of lowercase ASCII letters, digit runs, spaces and commas are the overwhelming majority of tokens. *Proposal:* a hand-written scanner for that common shape that falls back to the DFA on any character it does not fully understand, verified by differential fuzzing of token boundaries and types against the re2c scanner (ordinals, roman numerals, acronyms, periods, hyphens and URLs are the risk). *Est.:* 0.1-0.2 µs (1-1.5%). *Effort:* medium; correctness risk is the token *type* classification.

**N13 Carry lengths instead of recomputing `strlen`.** *address_parser.c:1672 (via the normalize wrapper), :1680, scanner.re:260 (`tokenize` calls `strlen(input)` again), plus per-token `strlen` in `context_fill`.* Several `strlen` calls on the same string per address. *Est.:* 0.03 µs (0.2%). *Effort:* trivial.

### Group P — context fill: token normalization and the three phrase searches (24% of time now)

**P1 Two-byte bitmap prefilter in front of every phrase search.** *trie_search.c:240 `trie_search_tokens_from_index`, called from address_parser.c:769/785 for the component-phrase and postal-code tries and from address_dictionary.c:299 for the address dictionary.* Each search walks every token from the root, and the misses fall off after one or two characters but only after reading cold trie nodes (this function is 12.6% own time in the current profile, the single largest function). *Necessary?* Only for tokens whose first two bytes can begin some phrase. *Proposal (greedy filter, niche data structure):* at load, enumerate each trie's keys (the enumerator exists) and build a 65,536-bit table (8 KB, L1-resident) of the first two bytes of every key; skip the walk for tokens whose pair is absent, including the single-byte-key case. *Exact:* built from the trie's own keys; no false negatives. *Est.:* the walks for non-matching tokens (most tokens) disappear: 4-7% of total time (0.5-0.9 µs). Experiment 012 showed prefetching these walks was slower, which is why this removes work instead of hiding it. *Effort:* medium; the search state machine (`SEARCH_STATE_*`, lines 240-470) must produce identical state for skipped tokens.

**P2 Same prefilter for the prefix and suffix dictionary searches.** *address_dictionary.c:338/355 `search_address_dictionaries_prefix/suffix`, trie_search.c:476 and :612.* Called per token, twice; the suffix search walks the word backwards with a `utf8proc_iterate_reversed` per character. *Proposal:* first-two-bytes bitmap for prefixes and last-two-bytes bitmap for suffixes. *Est.:* 1.5-2% (the searches, `utf8proc_iterate`, and `trie_get_node` calls under them are about 3% inclusive). *Effort:* small-medium.

**P3 One normalization loop for both token forms, for all ASCII tokens.** *address_parser.c:712 and :726, normalize.c:393 `add_normalized_token`.* Experiment 011 shares the two results only for digit-free ASCII tokens. `NORMALIZE_TOKEN_REPLACE_DIGITS` acts at exactly one place (normalize.c:457), so for any ASCII token the standard form is the admin form with each digit replaced by the digit character. *Proposal:* run once, derive the other in the same pass. *Est.:* +0.5% over what 011 already gets. *Effort:* small.

**P4 Stop calling `utf8proc_iterate` and `utf8proc_category` twice per character.** *normalize.c:393-505 (`add_normalized_token`): each character iterates the current and the next character and looks up both categories.* Carry "next" into "current" on the following iteration; for ASCII use a 128-entry class table. *Est.:* `normalize_token` is 4.3% inclusive now: save about 2.5% (0.3 µs). *Effort:* small.

**P5 Fill phrase memberships directly.** *trie_search.c:811 `token_phrase_memberships`.* Pushes one `int64` per token into a growing vector, in three separate arrays per address (plus `log_debug` no-ops). Fill in place. *Est.:* 0.3% (0.04 µs). *Effort:* trivial.

**P6 Precompute "token is all digits" once.** *address_parser.c:675 `is_valid_component_phrase` calls `strlen` and `string_is_digit` (string_utils.c:683, `utf8proc_iterate` plus category per character) for tokens inside component phrases, per phrase.* The normalizer already visits every character; record the flag there. *Est.:* 0.3% (0.04 µs). *Effort:* small.

**P7 Turn `is_valid_dictionary_phrase` into a table lookup.** *address_parser.c:851; called per token in a phrase (feature code) and again inside `word_or_phrase_at_index` (883-), and it loops every expansion of the phrase calling `address_expansion_in_dictionary`.* It is a pure function of the expansion index. *Proposal (DP/memoization):* a boolean array over `address_dict->values` computed once at load. *Exact:* same function of the same immutable data. *Est.:* 0.6% (`is_valid_dictionary_phrase` 0.3% + `address_expansion_in_dictionary`). *Effort:* small.

**P8 Make the postal-graph probe cheap.** *graph.c:92 `graph_has_edge`, called up to twice per postal-code token (address_parser.c:160, :1293, :1304); 2.1% own time in the current profile.* A binary search over a row of a ~500 MB CSR (cold cache lines at every step). *Proposal:* prefetch `indptr[postal_id]` and the row when the postal phrase is found (before the other work of the token), or store rows in Eytzinger order for cache-friendly probing, or put a small per-row bitset filter in front. *Exact:* same set membership. *Est.:* 1-1.5% (0.15 µs). *Effort:* small for the prefetch, medium for the layout.

**P9 Merge the three token walks.** *address_parser.c:760-790 runs `search_address_dictionaries_tokens_with_phrases`, then the component-phrase search, then the postal-code search over the same tokens.* With P1 in place a single per-token first-pair classification can gate all three. *Est.:* +0.5% beyond P1. *Effort:* small once P1 exists.

**P10 Aho-Corasick or radix-compressed phrase index (long-term alternative to P1).** The phrase and address-dictionary tries store multi-token phrases as space-joined strings walked byte by byte. A token-level automaton (each distinct token an alphabet symbol, phrases as paths, failure links for the backtracking that `SEARCH_STATE_PARTIAL_MATCH` implements by re-scanning) does one pass over the tokens. *Exact* only if it reproduces the longest-match and restart behaviour of the state machine exactly; that state machine has unusual cases (ideographic characters, hyphen-as-space, tail nodes). *Est.:* could remove most of the 15% inclusive phrase-search cost, but at high risk. *Effort:* high; do it only after P1 shows what is left.

**P11 Skip `normalized_admin` copies that equal the standard form.** *address_parser.c:726 and the `cstring_array` it fills.* After P3 the admin array is a byte-identical copy for most tokens; the phrase searches could read the standard array directly and only materialize the admin form for tokens where they differ. *Est.:* 0.4% (one array and its growth). *Effort:* small-medium (pointer plumbing).

### Group F — feature generation (19% of time now)

**F1 Compute each token's word descriptor once (dynamic programming across the token sequence).** *address_parser.c:883 `word_or_phrase_at_index`, :347 `word_vocab_frequency`, :1482-1530.* For token `t` the code computes the "word or phrase" descriptor, its vocabulary frequency and the unknown-word substitution for `t` itself, again as the *previous word* of `t+1` and again as the *next word* of `t-1`: three times per token (the vocab lookup is 3.7% inclusive now, phrase string rebuilds another share). *Necessary?* Once per token per address. *Proposal:* an array of descriptors filled in one left-to-right pass, read by all three roles. *Exact:* same pure function of the same inputs. *Est.:* 3-4% (0.4-0.5 µs). *Effort:* medium (the function is long and the descriptors depend on `long_context`).

**F2 Memoize phrase features per phrase.** *address_parser.c:1150-1260.* Every token of a multi-token phrase rebuilds the phrase string (`cstring_array_get_phrase`, trie_search.c:839) and emits ~20 `add_phrase_features` keys (lines 1017-1025, 1183-1192, 1213-1222); the keys are identical for all tokens of the phrase (they are the source of the 12.7% exact duplicate lookups measured in 001/002). *Proposal (memoization):* build the phrase feature list once per phrase and replay it (strings or, after lookup, ids). *Exact:* the features are functions of the phrase only. *Est.:* 3-5% (feature building plus the duplicate dictionary lookups, 0.4-0.7 µs). *Effort:* medium.

**F3 Resolve constant keys at load.** *address_parser.c:1338 `feature_array_add(features, 1, "bias")` etc.* `bias` is 5.7% of all state lookups and its key, id and weight row never change. *Proposal (greedy precomputation from the model):* resolve the id once at load and add its weight vector to every token's initial state row (the same additions in the same order, so the sums are identical apart from the sign of a zero, which cannot change any comparison). Same for other fixed-literal features that carry no value. *Est.:* 1-2% (0.13-0.25 µs). *Effort:* small.

**F4 Skip building strings for keys that are guaranteed misses (needs the model).** For each feature *name* the model has a known set of stored values; for names whose value set is tiny (for example `commonly city`) the value could be checked against a small array before hashing. *Est.:* under 0.5%. *Effort:* medium; low priority.

**F5 Hash while building the key.** *features.c:4 `feature_array_add`, then crf.c `crf_dict_lookup_array` / feature_dict.h `fd_split` (2.5% + `memchr` 1.4% + `memcmp` 3.4% in the profile).* The key is built into a string and then re-scanned for `|` and re-hashed. The call site already knows the name piece. *Proposal:* compute the name id and the hash of the value part as the pieces are appended and store both beside the key. *Exact:* same key bytes, same hash function. *Est.:* 2-3% (0.3-0.4 µs). *Effort:* medium.

**F6 Cache name ids by call-site.** *feature_dict.h `fd_find_name` (hash of the 4-40 byte name per key).* 29 names in the whole model. Names are string literals at almost every call site; cache `pointer -> id` with a one-time content check. The one dynamic name (`feature_prefix` at address_parser.c:1051) is checked by content. *Est.:* part of F5's gain. *Effort:* small.

**F7 One pass for the n-gram features.** *address_parser.c:1027 `add_ngram_features`, ngrams.c:4 `add_ngrams`.* For each rare word it is called four times (n = 3..6): each call does `sprintf("%zu", n)`, clears the array, re-walks the word with `utf8proc_iterate`, and builds each gram with several appends. *Proposal:* constant strings "3".."6", one pass producing all four gram sizes, ASCII path without `utf8proc`. *Est.:* 0.5-1% (only rare and unknown words). *Effort:* small-medium.

**F8 Find hyphens with `memchr` for ASCII.** *string_utils.c:735 `string_next_hyphen_index`, called in a loop for every rare word (address_parser.c:1400-1440).* It decodes every character with `utf8proc_iterate`. Hyphens outside ASCII have lead bytes at or above 0x80 (U+2010 to U+2015 and others), so scan for `-` and for any byte at or above 0x80, and only decode when one is found. *Est.:* 0.3% (0.04 µs). *Effort:* small.

**F9 Cache the phrase strings `cstring_array_get_phrase` builds.** *trie_search.c:839, called from `word_or_phrase_at_index` (three roles) and the feature function.* Covered by F1/F2; listed separately because it is also a per-call `char_array_clear`, per-token `char_array_append` and terminate. *Est.:* included in F1/F2.

**F10 Do not run the long-context loop when it cannot fire.** *address_parser.c:1560-1640, first token only.* It calls `word_or_phrase_at_index(..., long_context=true)` for every right-hand token until a break. Bound: only tokens up to the first number and first street phrase matter; the loop already breaks, but the descriptors it computes can come from F1's array. *Est.:* 0.3% (first tokens only). *Effort:* small once F1 exists.

**F11 Reuse feature hashes between the state and transition tries.** *address_parser.c:1467-1500: `"word|w"` and `"prev word|w"` are emitted for both the state features and the previous-tag features.* Both lookups hash the same value bytes with different name ids; compute the value hash once. *Est.:* 0.3%. *Effort:* small.

### Group C — CRF scoring and Viterbi (about 15% of time)

**C1 Store each weight row next to its key.** *crf.c weights lookup, feature_dict.h.* A hit needs the slot line, the key line, `indptr[id]`, then `indices[...]` and `data[...]`: four or five cold lines per hit lookup (the weight-accumulate line is 8.7% own time). *Proposal (layout transformation):* at load, build each dictionary entry as `[name][value][NUL][nnz][(class:1B, weight:8B) x nnz]` in the same order as the CSR row, then free the CSR arrays. Classes are below 256 (20 classes), so an index costs 1 byte instead of 4; rows average 2.9 entries, so a whole entry fits in one or two cache lines. *Exact:* identical doubles, identical order. *Est.:* removes about two of four or five cold lines per hit: 5-8% (0.65-1.0 µs). **Memory falls by roughly 120 MB** (indices 129 MB to 34 MB, `indptr` 44 MB removed). The same applies to the transition weights (161 MB). *Effort:* high (load-time rebuild, peak memory while both exist must be managed by consuming rows in id order and returning pages with `madvise`).

**C2 Undo instead of zeroing 19 KB of `state_trans` per address.** *crf.c `crf_tagger_score` (memset 1.8% own time), crf_context.c:101.* The matrix is `tokens x 20 x 20` doubles, zeroed every address, while only ~9 rows of ~6 entries per address are ever written. *Proposal (sparse undo):* keep the matrix permanently zero, record the cells touched by the accumulate loop, zero exactly those after decoding. *Exact:* same values seen by Viterbi. *Est.:* 1.5% (0.2 µs). *Effort:* small.

**C3 Alias the transition matrix instead of copying it.** *crf.c:25 `double_matrix_copy(self->trans_weights, crf_context->trans)`.* 3.2 KB copied per address; `trans` is only read by Viterbi. Point the context at `trans_weights` once. *Est.:* 0.4% (0.05 µs). *Effort:* small (ownership at destroy).

**C4 Viterbi without per-element accessor calls (keeping the loop order).** *crf_context.c:567-650: `state_trans_score(self,t,i)` and `trans_score(self,i)` are called inside the innermost loop, 800 calls per token.* Experiment 012's rewrite also swapped the loop order and came out slower (-2.7%). The cheap version keeps the exact `j`-outer, `i`-inner order and only hoists the row pointer for the current `t`; a further version uses NEON across `j`. *Est.:* 1-2% (0.13-0.26 µs; Viterbi is 5.2% now). *Effort:* small; measure per variant.

**C5 Skip Viterbi's zeroing of `backward_edges`.** *crf_context.c:101-125.* Small (T x 20 words), kept for the pathological -inf case; low value. *Est.:* 0.1%. *Effort:* trivial.

**C6 Keep class ids instead of label strings until the end.** *crf.c:116 `crf_tagger_predict`: `cstring_array_add_string(labels, predicted)` copies the class name string for every token; address_parser.c:1804 then compares strings with `strcmp` per token.* Compare ids; look the strings up only for the response. *Est.:* 0.5% (0.07 µs). *Effort:* small.

**C7 Prefetch the dictionary slot when the key is built.** *features.c:4 / crf_dict_lookup_array.* Today all keys are hashed in a batch pass and then probed. Issue the slot prefetch at `feature_array_add` time so the miss overlaps the *following* feature generation. *Est.:* 1-2% (memory-level parallelism gained without more instructions). *Effort:* small; needs F5.

**C8 Unroll the accumulate loop for short rows.** *crf.c weights accumulate.* Rows average 2.9 entries; a switch on row length with straight-line code removes loop overhead and branch mispredicts (mispredicts on the variable trip count are plausible). *Est.:* 0.5-1%. *Effort:* small; verify by measurement.

**C9 Order the accumulation by slot to help the TLB.** After C1, batch-sort hits by arena address before accumulating. Speculative. *Est.:* under 1%. *Effort:* medium; low priority.

**C10 Dictionary-code the weight doubles.** *33.8 M and 13.4 M non-zeros.* If the number of distinct double values is small (unknown; measure), store 16-bit codes and a table. Exact (same doubles). *Est.:* memory only; speed neutral. *Effort:* medium; only if memory is needed for other items.

### Group R — response, allocation and freeing (about 8-9% of time)

**R1 Build the response in one allocation.** *address_parser.c:1783-1841, string_utils.c:1334 `cstring_array_to_strings`, libpostal.c:241 `libpostal_address_parser_response_destroy`.* Each parse creates three `cstring_array`s (`token_labels`, `labels`, `components`: a struct and two vectors each), then `cstring_array_to_strings` `strdup`s every string (a `strlen`, a `malloc` and a copy each) and frees the arrays; the caller later `free`s every string separately. *Necessary?* The API contract is "call `libpostal_address_parser_response_destroy`", which is library code we control. *Proposal:* compute the total size, allocate one block for the `char*` arrays and all strings, and let destroy free the block. *Exact:* same strings. *Est.:* `cstring_array_to_strings` 1.8%, destroy 0.6%, the `cstring_array_new*` and `strdup` allocations about 2%: total 3-4% (0.4-0.5 µs). *Effort:* medium; depends on nobody else freeing components individually.

**R2 A bump-pointer arena for everything else that is per address.** *tokens, tokenized string, token labels, `ngrams`, the phrase arrays.* The profile shows the allocator at 5.7% inclusive plus zeroing 2.3%, with `cstring_array_new`, `cstring_array_add_string`, `cstring_array_to_strings`, `tokenized_string_destroy` and `address_parser_parse` all in the caller list. *Proposal:* one arena per parse, reset at the end. *Est.:* 3-4% (0.4-0.5 µs). *Effort:* medium.

**R3 Presize vectors that are created per address.** *vector.h:6 `DEFAULT_VECTOR_SIZE 8`, `_push` doubling.* Arrays created per address start at 8 elements and grow by doubling with `realloc` (the `xzm_realloc` and its `memmove` are 1.2% own time). *Est.:* 0.5-1%. *Effort:* trivial.

**R4 Compare class ids in the response builder.** covered by C6.

**R5 One block for a `cstring_array`.** *string_utils.c:1103 `cstring_array_new`: three mallocs per array.* Covered by R2.

**R6 Build the response strings straight from the normalized buffer.** *address_parser.c:1799-1825.* Components are assembled by appending each token string, with a space if the tokens were not adjacent, into `components`, then copied again. Precompute the final length and write once. *Est.:* 0.5% beyond R1. *Effort:* small.

### Group L — load time, model layout and memory (also makes room for the items above)

**L1 Compact exact representation of the postal-code graph.** *graph.c, address_parser.c:160; the ~500 MB (loaded from a 621 MB file whose trie is only 105 MB).* Rows are sorted admin ids: delta-code them with small integers or use per-row bitsets, keep `graph_has_edge` exact. *Est.:* memory -100 to -300 MB (unverified until measured); lookup cost neutral or better. *Effort:* medium.

**L2 `mmap` the model files.** *trie.c `trie_read`, sparse_matrix reads, file_utils.* All model arrays are `fread` into freshly `malloc`ed buffers (1.8 GB of files). Page-aligned zero-copy arrays remove the copy, cut load time (3 to 15 s) and shrink peak memory. *Est.:* load time -50% or more; peak footprint down. *Effort:* medium; on-disk formats must stay unchanged, so arrays must already be aligned or the reader must copy small headers only.

**L3 Do not set up modules the parser never uses.** *libpostal.c:332 `libpostal_setup` loads transliteration, numex and the address dictionary.* Transliteration and the address dictionary are needed; numex is not for parsing. *Est.:* small load time. *Effort:* trivial.

**L4 Hot/cold ordering of large tables.** The weight rows and dictionary entries for the most common feature names could be laid out together, chosen from the model itself (the vocabulary counts) and never from the benchmark. Layout only; results cannot change. *Est.:* 1-2%, uncertain. *Effort:* medium.

**L5 Prefault and page-size hints.** macOS arm64 uses 16 KB pages with no user superpages; `madvise` willneed on the dictionary and weight regions can move first-touch page-in out of the timed path (the first pass is 0 to 3% slower today). *Est.:* cold pass only. *Effort:* small.

### Group X — compiler, linker and platform

**X1 PGO and LTO.** The build is `-O2` without LTO; small functions defined in other translation units appear as separate frames in the profile (`trie_get_node` 1.1-1.3% own, `cstring_array_get_string`, `utf8proc_category`, `char_array_append`). Profile-guided optimization trained on the development split only and evaluated on validation and the novel set (rule 1 of the generalization protocol) plus link-time inlining. *Est.:* 5-10% (0.65-1.3 µs), high variance; and it needs the build recipe to be accepted. *Effort:* small to try, moderate to package.

**X2 Remove libc stub overhead.** *the profile shows `DYLD-STUB$$strlen` 0.8% and `DYLD-STUB$$memcpy` 1.1%, plus `_platform_memmove` 7.3% and `_platform_strlen` 3.7%.* Many tiny calls. Inline fixed-size copies, carry lengths, and avoid `strlen` on literals. Experiment 012's inline compare was slower, so this needs measurement per site. *Est.:* 1-3%. *Effort:* small per site, uncertain.

**X3 Mark hot tiny functions `static inline` in headers.** *`inline` functions defined only in `.c` files (for example `cstring_array_get_string`, `trie_get_node`, `char_array_terminate`) cannot be inlined across translation units without LTO.* Move to headers. *Est.:* 1-2% without LTO, subsumed by X1. *Effort:* small.

**X4 Compile flags for the target.** `-mcpu=native` / `-mcpu=apple-m4`, `-fno-semantic-interposition`, `-fomit-frame-pointer`. *Est.:* 0-2%. *Effort:* trivial; portability caveat if the build machine differs.


### Additional items found while re-reading

**P12 Compare tails with `memcmp` of known length.** *trie_search.c:302-305 (`strncmp((char *)current_tail, ptr + 1, ptr_len)`) and :560-580 (`strncmp`, `strlen` on the tail).* The lengths (`ptr_len`, `tail_remaining`) are already known; the library `strncmp` also scans for NULs and `strlen` re-derives a length. `_platform_strncmp` is still 1.0-1.1% of the current profile. *Exact:* equal bytes for the same lengths. *Est.:* 0.5-1%. *Effort:* small; per-site measurement (012 showed inline compares can lose).

**F12 Use the token lengths that already exist.** *address_parser.c:1121 `size_t word_len = strlen(word)` and other per-token `strlen` calls in the feature function (`_platform_strlen` 3.7%; `address_parser_features` and `word_vocab_frequency` are among its callers).* `normalized_tokens` already stores each normalized token's length (address_parser.c:712-724). *Est.:* 0.5-1% (0.07-0.13 µs). *Effort:* small.

**C11 Hoist the per-token accessor calls in scoring.** *crf.c weights loop: `state_score(crf_context, t)` and `state_trans_score_all(crf_context, t)` are out-of-line calls once per token, and `double_matrix_get_row` inside them.* Compute the row base pointer once. *Est.:* 0.2-0.4%. *Effort:* trivial.

**R7 Compare labels by id in the component builder.** *address_parser.c:1804 `strcmp(label, prev_label)` per token and `cstring_array_add_string(labels, label)`.* With C6 the labels are ids. *Est.:* 0.2%. *Effort:* trivial.

**L6 Cut the load-time conversion cost.** *feature_dict.h `fd_for_each_key`: the 256-wide child scan of every internal trie node (about 2.7 billion compares).* NEON compares four node `check` fields at a time (about 4x on that scan); the hash inserts can be prefetched. The build is 3.2 s of the 3.7 s of added setup time. *Est.:* load time -1.5 to -2 s; matters only if the evaluation times setup. *Effort:* small.

**X5 Avoid function-pointer and varargs cost on the hot path.** *features.c:4 `feature_array_add(features, count, ...)` is variadic (va_list handling) and the feature function is called through a `tagger_feature_function` pointer once per token.* Replace the variadic call with fixed-arity inline wrappers (`feature_add1`, `feature_add2`, `feature_add3`, `feature_add4`), removing `va_list` bookkeeping and letting the compiler see constant piece counts. *Exact:* same bytes. *Est.:* 0.5-1% (feature_array_add is 8.0% inclusive). *Effort:* small-medium (about 60 call sites).

## 4. Function-by-function audit (every function above 0.15% of the baseline profile, plus the setup path)

Columns: baseline inclusive share (own share in brackets where it matters) · necessary? · verdict (item IDs from section 3). "Done" means an accepted experiment already handled it.

| Function (file) | Baseline share | Necessary? | Verdict |
|---|---:|---|---|
| `libpostal_parse_address` (libpostal.c:274) | 99.7% | wrapper | Trivial pass-through; nothing to gain |
| `address_parser_parse` (address_parser.c:1661) | 99.6% | yes | Restructure allocations and copies: N9, N10, N11, N13, R1, R2 |
| `address_parser_predict` / `crf_tagger_predict` (crf.c:116) | 78% | yes | Label copies per token: C6 |
| `crf_tagger_score` (crf.c:15) | 75.8% (11.7% own) | yes | Weight rows, zeroing, aliasing, accessors: C1, C2, C3, C7, C8, C11 |
| `trie_get_data`, `trie_get_from_index` (trie.c) | 49% (39.6% own) | no longer needed for the CRF | **Done** (006 dictionary); vocabulary done (010) |
| `address_parser_features` (address_parser.c:1089) | 15.1% | yes | F1, F2, F3, F5, F10, F11, F12 |
| `address_parser_context_fill` (address_parser.c:687) | 10.2% | yes | P1 to P11 |
| `_platform_strncmp` (libc) | 7.9% | mostly trie tail compares | CRF share gone (006); rest P12 |
| `normalize_string_latin` (normalize.c:117) | 7.1% | yes, for non-ASCII | ASCII done (011); non-ASCII N1 to N7 |
| `feature_array_add` (features.c:4) | 6.6% | yes | Fast join done (010); X5, F5 |
| `trie_search_tokens_from_index` (trie_search.c:240) | 6.6% (5.4% own) | yes, but avoidable per token | P1, P9, P10, P12 |
| `char_array_add_vjoined` (string_utils.c) | 6.2% (5.2% own) | not on this path any more | **Done** (010) |
| `transliterate` (transliterate.c:669) | 3.6% | only when a rule can fire | N1 to N4 |
| `normalize_string_utf8_languages` (normalize.c:24) | 3.3% | yes | N5, N6 |
| `_platform_memset` (libc) | 2.7% | partly | C2 (undo zeroing), R2 |
| `_realloc`, `xzm_realloc`, `_malloc_zone_realloc` | 2.3% | avoidable | R3, R2 |
| `_platform_strlen` (libc) | 2.2% | avoidable | F12, N13, X2 |
| `normalize_token`, `add_normalized_token` (normalize.c:393,508) | 2.0% / 1.8% | yes | P3, P4 |
| `utf8_case` (string_utils.c:228) | 1.9% | replaceable | N5 |
| `crf_context_viterbi` (crf_context.c:567) | 1.9% | yes (the DP itself) | C4 |
| `utf8proc_decompose` / `_decompose_char` / `_reencode` / `_map` / `_iterate` | 1.8% / 1.0% / 0.9% / 1.4% / 1.0% | for non-ASCII | N5, N6, P4 |
| `crf_context_set_num_items`, `crf_context_reset` (crf_context.c:101,237) | 1.8% / 1.7% | partly | **Done** (005 dropped the reset, 010 lean context); C2 |
| `_xzm_free`, `_xzm_xzone_malloc`, `_malloc_zone_malloc`, `_free`, `xzm_malloc_zone_size` | ~2.3% | avoidable | R1, R2 |
| `trie_get_prefix_from_index` (trie.c) | 1.2% | recomputed constant | N3 |
| `graph_has_edge` (graph.c:92) | 1.2% | yes | P8 |
| `word_or_phrase_at_index` (address_parser.c:883) | 1.1% | computed 3x per token | F1, F9 |
| `next_prefix_or_set` (transliterate.c:119) | 1.0% | engine internals | N1, N2 (skip the steps) |
| `_platform_memmove` (libc) | 1.0% | partly | X2, R3 |
| `tokenize`, `tokenize_add_tokens`, `scan_token` (scanner.re:230,264,7) | 1.0% / 0.9% / 0.9% | yes | N12, N10 |
| `cstring_array_new_size`, `cstring_array_new`, `cstring_array_add_string` | 0.9% / 0.7% / 0.8% | per-address allocation | R2, R5 |
| `char_array_cat_len` (string_utils.c) | 0.7% | in transliterate/normalize | N4 |
| `cstring_array_get_phrase` (trie_search.c:839) | 0.6% | rebuilt per call | F1, F2, F9 |
| `cstring_array_to_strings` (string_utils.c:1334) | 0.6% | response | R1 |
| `char_array_append`, `char_array_append_len`, `cstring_array_start_token` | 0.6% / 0.2% / 0.5% | yes | **Done** (005 memcpy append) |
| `search_address_dictionaries_prefix/suffix` and the `trie_search_prefixes/suffixes_*` chain | ~2.3% total | for tokens that can match | P2 |
| `double_matrix_resize` (matrix.h:100) | 0.5% | reallocated on growth after shrink | **Done** (010 lean context) |
| `trie_get_node` (trie.c:107) | 0.5% (own) | tiny, called across TUs | X1, X3 |
| `add_ngram_features`, `add_ngrams`, `string_next_hyphen_index` | 0.4% + | rare and unknown words | F7, F8 |
| `utf8proc_category`, `utf8_is_hyphen`, `string_is_digit` | 0.4% / 0.2% / small | avoidable for ASCII | P4, P6, F8 |
| `token_phrase_memberships` (trie_search.c:811) | 0.3% | yes | P5 |
| `tokenized_string_new_from_str_size`, `_add_token`, `_destroy`, `strndup`, `strdup` | 0.3% / 0.2% / 0.2% / 0.2% | rebuilt each address | N10, N4 |
| `is_valid_dictionary_phrase`, `address_expansion_in_dictionary` | 0.3% / 0.2% | pure function of the index | P7 |
| `libpostal_address_parser_response_destroy` (libpostal.c:241) | 0.3% | yes | R1 |
| `cstring_array_get_string`, `trie_get_transition_index` | 0.2% each | tiny, cross-TU | X1, X3 |
| `mach_absolute_time` | 0.3% | harness timer, not the library | none |
| **Setup path** (`libpostal_setup` libpostal.c:332, `address_parser_load`, `trie_read`, `sparse_matrix_read`, `graph` load) | load time | yes | L2, L3, L5, L6 |

Also read and judged necessary with no change proposed: `token_array_push`, `phrase_array_push`, `phrase_at_index`, `add_phrase_features` (only through F2), `is_plain_word_phrase_type`, `postal_code_context_exists` (only through P8).

## 5. Can 3.5x be reached? The arithmetic

Target on the full real set: baseline 34.88 µs per address, so 3.5x is **9.97 µs**; the current best build is 13.22 µs, so **3.25 µs** (24.6%) still has to go.

Estimated savings by group, from section 3 (µs per address on average over the full set; low and high are my ranges, UNVERIFIED):

| Group | Low | High | Notes |
|---|---:|---:|---|
| N (normalization, tokenizing) | 1.4 | 2.1 | dominated by N1 to N6 (non-ASCII) and N10 |
| P (context fill, phrase searches) | 1.5 | 2.2 | dominated by P1, P4, P8 |
| F (feature generation) | 1.3 | 1.9 | F1, F2, F5; already discounted 20% for overlap |
| C (CRF scoring, Viterbi) | 1.3 | 2.0 | dominated by C1, C2, C4, C7 |
| R (response, allocation) | 0.6 | 0.9 | capped by the allocator's total 0.75 µs share plus response building |
| L, X (layout, compiler) | 0.3 | 1.9 | X1 (PGO and LTO) is the wide one |
| **Sum before overlap** | **6.4** | **11.0** | |
| **After a 30% overlap discount** | **4.5** | **7.7** | items touch the same code and stack on the same cold lines |

Turning that into an expectation. My estimates in this project have realized as follows: 005 predicted 10-14%, got 4.4% (0.35x); the vocabulary dictionary predicted 2%, got 6.7% (3.3x); the fast join predicted about 3%, got 3.7%; the lean context predicted 2-3%, got 1.8%; the arena prefetch predicted 2%, got -0.2%; the ASCII path predicted 5-8%, got 11%; experiment 012 predicted about 12% for three changes and got -2%. A realization factor of 0.35 (pessimistic), 0.6 (central) and 1.0 (as estimated) applied to 4.5-7.7 µs gives:

| Realization | Saved | Per address | Speedup on the full set |
|---|---:|---:|---:|
| 0.35 | 1.6-2.7 µs | 10.5-11.6 µs | 3.0x - 3.3x |
| 0.6 (central) | 2.7-4.6 µs | 8.6-10.5 µs | 3.3x - 4.0x |
| 1.0 | 4.5-7.7 µs | 5.5-8.7 µs | 4.0x - 6.3x |

3.5x needs 3.25 µs of realized savings. Against the after-overlap range that is **0.42 of the high estimate (7.7 µs), 0.53 of the midpoint (6.1 µs) and 0.72 of the low estimate (4.5 µs)**. Read plainly: if the items deliver at the average rate this project has seen, 3.5x is reached; if they deliver at the worst rate we have seen, we land at about 3.0x to 3.3x. So I do not think 2.6x is the end, and I do not think 3.5x is certain. The results that matter most are C1, N5/N6, F1/F2, P1 and R1/R2; the rest is margin.

**Ordering by (expected gain) / (risk and effort):**
1. **Wave 1, cheap and exact, each independently testable:** N11, N13, N9, N3, N1, N2, P5, P6, P7, R3, C3, C2, C11, F8, F12, P12, R7. Together an estimated 0.8-1.6 µs (6-12%).
2. **Wave 2, structural, the main gains:** F1 and F2 (memoization across tokens and phrases), P1 and P2 (prefilters), C1 (inline weight rows), R1 and R2 (one-block response, arena), N10 (tokenizer reuse), P8. Together 2.4-4.4 µs.
3. **Wave 3, the non-ASCII normalization:** N5 and N6 with the per-code-point table. 1.0-1.3 µs and it removes the dependence on the ASCII share of the data. Highest verification cost.
4. **Wave 4, uncertain, measure before building:** X1 (PGO and LTO), C4, C7, C8, F5/F6, X2, N12, L4, P10 (Aho-Corasick).
Each item is an experiment of the same shape as 003 to 012: byte-identical dumps on the full set and the novel set, the end-to-end fuzz, the controlled interleaved timing session, and a rebuild control (the control build has moved timings by up to 0.4%, so effects below about 1% are not resolvable without the paired 20-round protocol).

## 6. What is already known not to work (do not repeat)

- Explicit-lane interleaved trie walks (experiment 004): no gain over plain batching.
- Prefetching the arena entry before the key compare (010 `fE`): -0.2%.
- Inline hash tail and inline compare in the dictionary, a Viterbi loop nest with source label outermost, and prefetching the phrase-search second-level nodes (012): all slower than the control (-0.7%, -2.7%, -1.4%).
- Lesson: a profile share is an upper bound on what a rewrite saves. Most of the accepted gains came from *removing work or changing layout*, not from making the same instructions cheaper. Items in section 3 are ordered accordingly (memoization, prefilters, layouts first).

## 7. Deliberately excluded

Beam pruning or any approximation in Viterbi (changes output); single-precision weights (changes output); a result cache across addresses, or any table or ordering derived from the benchmark data (violates the generalization rule); parsing on more than one thread; multi-process tricks; skipping features for speed. Also excluded: anything whose exactness I cannot check against the original library on the full set, the novel set and the fuzz corpus.

## 8. How each item would be kept exact (the protocol, so a reviewer can judge the risk)

- **Table/prefilter items (N1, N2, N6, P1, P2, P7):** the table is built from the model's own data at load by the trie enumerator that already exists, with a completeness check (leaf count equals enumerated count); a fallback to the original code on any doubt.
- **Memoization items (F1, F2, P3, P7, N3):** the memoized function is pure in its inputs; tested by running old and new code side by side on the full set and on fuzzed inputs and comparing every intermediate.
- **Layout items (C1, R1, L1, L2):** the same numbers in the same order; exhaustive check that every stored row equals the CSR row it replaced (11.5 M and 2.1 M keys), plus the end-to-end dumps.
- **Normalization (N5, N6):** exhaustive over every Unicode scalar value for single-code-point behaviour; pair and triple tests over all classified categories; random strings of eligible code points; the original function stays as the fallback and as the oracle.
- **Every item:** golden-v1 and novel dumps byte-identical, 2 seeds x 2,000,000 fuzzed inputs identical, one thread, footprint not above the baseline, controlled interleaved timing on the whole real set, the floor set and eight random subsets.

## 9. Open questions worth one measurement each (no code needed to answer them)

1. Fraction of non-ASCII tokens that pass the N2 first-codepoint test (how much of the second transliteration step can be skipped).
2. Number of distinct weight values in the CRF (C10) and the row-length histogram (C8).
3. Share of tokens that survive the two-byte prefilters (P1, P2) in the real data; and the size of each bitmap.
4. Whether a memoized phrase-feature list (F2) is shorter than the ~20 keys the code re-emits.
5. Peak memory while C1 rebuilds rows in id order (whether the row build can stay under the baseline's peak).
