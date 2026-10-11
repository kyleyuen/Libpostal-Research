# Experiment 185 — operation coverage and report requirements

Status: instrumentation, runtime leaf-hook review, nested-scope and overflow calibration, known-case output checks, and the 39-record diagnostic collection are complete. Collection: `results/full13-retry3`; renderer output: `results/full13-retry3/REPORT.md`. Outputs matched the preserved original byte-for-byte and sampled single-thread evidence passed. Final acceptance and product performance were not evaluated; this audit makes no savings or speed claim.

## Scope and comparison identity

Compare same input bytes/order and model across the preserved original behavior (using a separately instrumented derivative), cpi/119, and 181. Keep each build's source and build identity in its provenance. The preserved original library remains untouched. A counter is usable across builds only when its semantic event and increment point match; otherwise mark that cell unavailable and explain the gap.

Historical counts are useful context, not substitutes for matched 185 baselines:

| Evidence | What it supports | Limitation for 185 |
|---|---|---|
| 029 | Stage and stage-family baseline for original and 021 `l4`; development split. | Different source/build from cpi/181; stage time is not operation counts. Whole-process instructions/cycles include load noise. |
| 030 | `l4` development: 54.6 feature lookups/address; 57.4% string path; 35,138 repeated string keys in corpus. | Its definition is feature-name lookup accounting on `l4`; does not cover context phrase search, vocab lookups, all cpi dictionary APIs, or 181's moved lookup. |
| 038 | `l3` repeated component/postal trie searches, feature section times. | Deliberate duplicate-search cost, not natural repetition counts; older source and development-only. |
| 040 | Warm extra feature-generation instructions/cycles. | Re-execution ablation, not operation counts; older `l3`. |
| 042/043 | Feature/weight work and cold-vs-warm split. 042's first warm-cost interpretation was corrected by 043. | Older `l3`; weight additions are required scored contributions, not redundant work. |
| 112/132 | xctrace CPU sample attribution for older `psf` and cpi on OSM. | Inclusive/sample shares overlap, profiler inflates time, not exact cycles or miss counts. |
| 180/181 | Direct state-word events/fallbacks and semantic preservation of carried IDs. 180 counted 29,608 direct + 3,509 fallback on golden and 500,808 + 52,287 on bounded fuzz. | Not global query counts, not controlled against matching original/cpi counts, and 180's per-event stderr is not a usable aggregate. 181 moves work across stages. |

## Coverage matrix

### Implemented source scopes (verified in the collected run)

The following counters are present in `source/{original,cpi,prototype181}/{address_parser.c,crf.c}`. They are scoped source events, not global libc/API totals:

| Scope | Metric / exact event | Labels and boundary | Limitation |
|---|---|---|---|
| Parse input | `input_bytes`, `normalized_bytes`, `tokens` | stages `normalization`, `normalization`, `tokenization`; parser input `strlen`, normalized result `strlen`, tokenizer token count | Does not count temporary normalization writes or token byte copies. |
| Feature-key materialization | `feature_keys_materialized`, `feature_key_bytes_materialized` | `feature_generation` or 181's `context_fill`; family `feature_key` / `transition_feature_key`; counted at CRF scorer key boundary, byte count excludes NUL | This is not total feature constructor calls/bytes. cpi integer-ID routes can bypass scorer key strings. Consequently original-vs-cpi differences describe text reaching this boundary only, not total logical feature requests. |
| 181 early direct-ID key | Same feature-key metrics for `crf_state_word_id`'s temporary `word|<token>` key | `context_fill`, family `feature_key`; bytes exclude NUL | The earlier `ap_front_crf_state_word_id` query boundary is separate from the later carried-ID consumption; report both direct-ID counts and actual lookup. |
| Model work | `model_rows`, `weights_accumulated` | state=`crf_state_lookup/state_feature`; transition=`crf_transition_lookup/transition_feature`; incremented at each resolved row and each scalar weight addition | These count scoring work, not duplicate-query savings or irreducible work. cpi's direct-ID paths still count rows/weights when they consume an ID. |
| 181 direct IDs | `direct_word_ids_offered`, `direct_word_ids_consumed`, `direct_word_fallbacks` | `feature_generation/direct_id` (early temporary key is separately `context_fill`) | Offered/consumed/fallback are flow events, not themselves model lookups. Reconcile to leaf search counts and row events. |
| cpi prefix/suffix flags | `filter_checks`, `searches_avoided` | `phrase_search/prefix_suffix_flag`; two checks per token; avoided count only for exact negative flags that skip their corresponding searches | The recorded final 119 cpi switch set enables `EXP_PS_FLAGS`; report these as cpi-only and N/A for original/181. Calibrate checks, flagged skips and leaf searches together. |
| Parser allocations | `diag_alloc.h` macro-wrapped malloc/calloc/realloc in parser and CRF translation units | `parser_alloc` and `crf_alloc`, active parse interval | Excludes other translation units, shared collection/string helpers, process setup, and diagnostic runtime allocations; requested allocation bytes are not live heap or RSS. |
| Output construction | `output_bytes` | `response_build/parser_output`, sum of response component and label bytes | Excludes external serializer/dumper. |
| Search stage tags | Word vocab, whole phrase/address, prefix/suffix, component and postal wrappers set/restore stage/family/operation; actual leaf calls are recorded once | `word_vocab_lookup`; `address_phrase_tokens`; `address_prefix`; `address_suffix`; `model_phrase_tokens`; stages per the approved stage set | Prefix/suffix counters are available only for cpi's enabled `EXP_PS_FLAGS` filter path. Phrase traversal edges and full bytes examined remain unavailable. |

Remaining constructor-side gap: `feature_array_add`/feature constructors are not fully counted. For 181, include the explicit `ap_front_crf_state_word_id` early key and distinguish it from scorer-boundary keys; other front-half temporary string construction is not currently measured. Mark total generated logical feature requests and all intermediate text as unavailable unless later hooks establish exact counts.

### cpi batch audit updates

The source scopes cpi's state and transition dictionary/trie batch calls separately, including save/restore of stage, family, and operation around each batch. The exact-key duplicate branch in `crf_key_batch_lookup()` emits `diag_reuse` when it copies a prior batch ID and skips a leaf search. It does not classify a first-use precomputed ID as a reuse. Calibration and the matched run recorded zero `answers_reused` in all three variants, so no same-address batch answer reuse was observed by these hooks.

Correction to the flag set: the final 119 cpi switches do enable `EXP_ROLE_IDS` and `EXP_CP_IDS`. The current instrumentation does not count cpi component-phrase IDs pushed from the `EXP_CP_IDS` table or role IDs pushed by the `EXP_ROLE_IDS` route as separate logical feature requests/ID consumptions. These are first-use precomputed model IDs, not same-address query reuse, and must not be added to `answers_reused`. Describe those source paths and mark their logical request/consumption counts unavailable. `EXP_PS_FLAGS` is enabled and its cpi-only check/skip counters are available; original and 181 have no matching flag path.

Instrument actual executed operations at the underlying search or construction boundary, not feature request/routing boundaries alone. Report raw totals and per-address and per-token rates, by stage and relevant family. For distributions, include per-address p50/p90/max where recording them does not require large traces.

| Family / boundary | Count and classify | Source map and review notes | Priority |
|---|---|---|---|
| Phrase dictionary searches | Calls, hit/miss, tokens/edges visited, bytes examined for whole-address, component, postal, prefix/suffix, and address-dictionary searches. | `address_parser_context_fill`: original/cpi around `address_parser.c:957–1160`; 181 around `:738–900`. Include actual APIs below `search_address_dictionaries_*`; avoid counting a routed/memoized request as a search. | Required |
| Vocabulary/model word lookup | Actual vocab-trie/dictionary searches, hits/misses, and requested-word identity hashes sufficient to count same-address repeats without storing full strings. | `word_vocab_frequency` at original/cpi `address_parser.c:483` and 181 `:380`; called from context fill and feature generation (notably 181 `:1380,1478,1546,1572`). Count by site/stage so movement is visible. | Required |
| State feature ID lookup | Actual trie/dictionary lookup calls, hits/misses, probes/comparisons/bytes where each structure supports it; key source/family (word, phrase, postcode, other). | Original/181 CRF helper `crf_get_feature_id` and call sites in scorer (`crf.c:7,148,171` etc.); cpi uses `fd_lookup`/`fd_lookup_hashed*`, batched resolvers and integer-key paths (`crf.c:57–68,182–235,434–532,851`). Count at leaf search API and retain logical feature-family attribution. |
| Transition feature ID lookup | Same metrics as state IDs, separate dictionary and feature family. | `crf_get_state_trans_feature_id`; cpi parallel dictionary APIs. Do not merge state/trans totals: different tables and query populations. | Required |
| Per-address duplicate/reuse | Repeated logical query count for identical structure + key bytes + semantics, first occurrence vs repeat; answers served from memo/table without search. | Compare queries over the entire parse, including context fill, feature generation, and scorer. Track query identity (e.g. stable hash plus length and collision-safe equality when traced) and separate *requested* query from actual search. | Required |
| Feature request to model query chain | Requests generated, request-to-ID resolutions, misses, bypassed/no-feature events, and event order. | Feature construction (`address_parser_features`) → CRF `crf_features_accumulate` / scorer → state/trans lookup → row accumulation. cpi can resolve/batch or directly emit IDs; expose equivalent logical request counts and actual lookup counts. | Required |
| Text normalization and tokenization | Input/normalized bytes; normalization calls/allocations/bytes emitted; token count and token bytes; fallback when normalization returns null. | `address_parser_parse` and normalization/tokenization around original/cpi `address_parser.c:2444+`; 181 `:1720+`. Keep normalized output separate from intermediate lookup-key text. | Required |
| Phrase and feature key text | Keys/strings created by family, bytes appended/copied, hash input bytes, bytes compared. Split intermediate key construction from parser output strings and dumper serialization. | `feature_array_add` / feature generation in address_parser; CRF key builders and `crf_state_word_id` in `crf.c:65` (181). cpi's integer/batched paths can avoid text; record bypassed key bytes as zero only when no text was actually created. | Required |
| Other text movement | `memcpy`/`memmove`/append at documented parser sites; input and output bytes. | Focus on parse path, `cstring_array`, normalization and response creation. Do not globally wrap libc without separating model load, harness/dumper, and parser boundaries. | Useful |
| Allocation and capacity growth | Calls and requested sizes by parser-owned allocation site/type; realloc old/new capacity, growth events, temporary high-water/live bytes when safely supported. | Include normalization/token/context/features, CRF temporary event/key arrays, and response construction. Exclude setup and output dumper, or report them separately. Separate cumulative requested bytes from live bytes/OS footprint. | Required |
| Model scoring rows/weights | State and transition row visits, weights actually added, skipped/missing rows, per-label contributions if compactly aggregated. | cpi `crf.c` accumulation loops near `:670–710`; cpi historical 132 shows ~1,100 adds/address but this must be re-collected on matched input. Distinguish required repeated contributions from duplicate query work. | Required |
| Filters and fallbacks | Every predicate check/pass/reject; exact searches skipped due to it; fallback entries and their actual downstream searches/bytes/allocations. | Examples: 181 front record `ap_front_fill` and fallback feature path; cpi integer keys, phrase-ID guard paths, flags and direct-ID fallback. A routed event is not a search avoided unless the prior equivalent search is documented. | Required when present; otherwise explicit N/A |
| Output response construction | Component/label strings and bytes, allocation calls, response block copies. | Parse API response only. Keep separate from feature/intermediate strings and external JSONL dumper. | Useful |

## Model-query chain and equality requirements

For a token, 181 `ap_front_fill` first reads its normalized word, calls `word_vocab_frequency`, then conditionally calls `crf_state_word_id` when vocabulary frequency is nonzero. That ID resolver builds `word|<token>` and calls `crf_get_feature_id`. Later feature generation consumes the carried ID through direct event flow or falls back to the original string feature and CRF lookup. In cpi, integer-key paths and precomputed phrase IDs may answer directly; count their logical feature requests, actual dictionary probes/searches, and model-load-only table construction separately. The reference parser takes the word/phrase feature routes at generation and resolves feature keys at the CRF stage.

Do not call `word_vocab_frequency` and CRF state lookup redundant just because both concern a word: they query different model structures and the state lookup is conditional. Do not call each repeated label/weight addition redundant: each is a distinct scored contribution needed by the model. Candidate reuse is plausible only for equivalent query semantics and lifetimes; source audit alone cannot say how frequent or costly it is.

For repeat equality, dictionary identity + exact key bytes/length + lookup semantics must agree. A matching 64-bit fingerprint can screen repeats, but collision handling must preserve correctness (e.g. trace a bounded sample for exact comparisons). Count reuse only when a prior answer is consumed and no underlying search executes. Retain per-stage totals and whole-pipeline totals to catch moved operations.

## Hardware/profile feasibility from local evidence

The existing macOS evidence supports `xctrace` CPU Profiler sampling and `/usr/bin/time -l` whole-process resource reporting. Historical records explicitly state that hardware cache/TLB miss counters were not collected; stack/sample shares do not establish them. The 185 records do not contain PMU events for cache misses, stalls, or memory latency, so those remain unavailable. No inference from profile shares is made. The collected operation diagnostics are not product timings. Golden-v1 RSS for every experimental release build exceeded the preserved original in this collection; build configurations differ, and this audit does not establish compliance with the user's memory cap or eligibility for promotion.

## Measurement report contract

`REPORT.md` should contain, for every corpus and each of original/cpi/181:

1. Build/source/model/input identity and matched-source instrumentation status.
2. Raw totals and normalized `/address` and `/token` values with exact coverage denominator.
3. Lookup family/stage tables: calls, hits/misses, repeat requests, actual repeats, answers reused, traversal/probe data, and explicit unavailable cells.
4. Text bytes/keys, allocation calls/bytes/growth and available high-water values; make scope boundaries explicit.
5. Model rows/weights, filter decisions, avoided searches and fallbacks. Separate required scoring events from potentially shareable lookup work.
6. Instrumentation memory/storage and output equality evidence; report diagnostic timing only as overhead/provenance, never as product speed.
7. Cross-stage reconciliation: per-family whole-pipeline totals and any moved query explanation. No savings conclusion without matched counts, no speed conclusion without the uninstrumented final benchmark.

If metrics are missing or collection failed, report them as unavailable with reason, never zero. Keep byte output and single-thread checks as hard gates from `METHOD.md`; a clean diagnostic does not replace full acceptance validation.

Repeated-pattern rows are the first 128 distinct repeated groups encountered by input order, not globally most frequent patterns. Describe them as examples only. Make broad pattern claims from the aggregate per-stage/family totals and all-query repeat counts; report query-table overflow explicitly because it makes exact repeat totals unavailable and retained rows incomplete.

Scalar repeat identity is exact within a run: dictionary pointer + operation semantics + exact key bytes/length. Exported retained-pattern rows intentionally aggregate by semantic family + operation + key bytes and omit dictionary identity; same-named dictionaries can therefore merge in those illustrative rows. Do not infer cross-dictionary shareability from a merged pattern row. Global claims should use scalar coverage/repeat totals, not sample pattern IDs.
