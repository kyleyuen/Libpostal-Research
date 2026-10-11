# Experiment 185 — operation diagnostics

Run: `full13-retry3`  
Build set: `20261007-1625-pattern-summary-fix`  
Status: diagnostics collected; final acceptance not evaluated  
Window: 2026-10-07T15:35:58.782126-07:00 to 2026-10-07T15:46:24.354517-07:00

This report describes operation counts, not speed. Each corpus is reported separately. Process durations are provenance only; diagnostic instrumentation adds overhead.

## Verified collection summary

The run contains all 39 expected dataset/variant pairs: 13 corpora × original, cpi, and prototype181. All 39 release and diagnostic outputs are byte-identical to the preserved original output; sampled thread evidence reports one thread in every record. For the three golden-v1 records, the frozen ordered-component comparison also passed, and all three negative controls were detected. Every per-address query table stayed within capacity, and each repeated-pattern summary reconciles retained and omitted observations exactly. The run status is still `diagnostics collected; final acceptance not evaluated`; this diagnostics experiment makes no performance claim.

The totals below sum operation counts over 128,540 address executions per variant across the 13 corpora. The corpus tables remain separate because input distributions differ. They are not timing aggregates.

| Variant | Lookup calls | Hits | Misses | Repeated executed queries | Feature keys at scorer boundary | Feature-key bytes | Model rows | Weights accumulated | Allocation calls / requested bytes | Filter checks / searches avoided |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 9,128,334 | 7,040,947 | 2,087,387 | 1,147,591 | 6,256,862 | 130,316,829 | 5,530,942 | 130,634,561 | 128,564 / 3,085,152 | N/A |
| cpi | 3,350,544 | 2,176,631 | 1,173,913 | 399,036 | 1,930,029 | 26,714,815 | 5,530,942 | 130,634,561 | 385,767 / 30,031,553 | 1,377,634 / 847,683 |
| prototype181 | 9,528,504 | 7,256,719 | 2,271,785 | 1,177,328 | 6,657,015 | 134,838,646 | 5,530,942 | 130,634,561 | 807,956 / 10,936,663 | N/A |

Interpretation is bounded to these counters. cpi records fewer lookup calls and fewer scorer-boundary feature strings than original; its filter counters show checks and searches skipped on the enabled `EXP_PS_FLAGS` path. Model row and weight totals are identical across variants, so the measured scoring workload counts are unchanged. prototype181 records more lookup calls than original. These counts do not establish elapsed-time or memory savings. `answers_reused` is zero in all three variants; its scope is same-address batch deduplication, and direct/precomputed IDs are not treated as reuse.

Repeated query totals count a repeated executed search after its first occurrence; they are not cache-hit counts. The exported pattern rows are bounded illustrations: the first 128 encountered semantic family/operation/key groups per record, not the most frequent groups. Scalar query identity also includes dictionary pointer, while rows merge equal semantic family/operation/key bytes across dictionaries. For example, original golden-v1 reports 87,139 distinct per-address repeated groups and 225,321 repeated-group observations, of which 47,621 groups / 114,454 observations fall beyond the retained-row cap. Do not extrapolate the examples into a global shareability rate.

Examples from the first encountered retained rows in golden-v1 (not top-frequency patterns):

| Variant | Family / operation | Stages | Query observations | Address groups | Reuse answers |
| --- | --- | --- | ---: | ---: | ---: |
| original | `word_vocab` / `word_vocab_lookup` | `feature_generation` | 6,372 | 2,307 | 0 |
| original | `state_feature` / `state_feature_lookup` | `crf_state_lookup` | 32,583 | 11,564 | 0 |
| cpi | `word_vocab` / `feature_vocab_canon` | `feature_generation` | 3,414 | 1,699 | 0 |
| prototype181 | `word_vocab` / `word_vocab_lookup` | `context_fill`, `feature_generation` | 6,372 | 2,307 | 0 |

Query observations for a retained row include all executions belonging to a repeated per-address group, including its first execution; they are not the `repeated_queries` scalar definition. The full bounded rows and hashes are in [the raw result records](results.json).

Golden-v1 peak RSS evidence: the preserved original executable measured 2,362,228,736 bytes; the original release candidate measured 2,612,789,248 bytes and its instrumented sibling measured 2,612,330,496 bytes. CPI release and diagnostic siblings measured 2,820,374,528 and 2,818,146,304 bytes; prototype181 siblings measured 2,612,723,712 and 2,612,461,568 bytes. Every experimental release build's golden-v1 RSS exceeds the preserved original in this collection, so this audit does not establish compliance with the user's memory cap and none is eligible for promotion on current evidence. These process resource observations are not a product memory conclusion: compared builds have different configurations, no configured RSS acceptance threshold is evaluated here, and the diagnostics are separately instrumented.

## Per-corpus counters

Counts are shown as total plus per-address and per-token rates where applicable. In scalar tables, `N/A` means the runner marked the scope unavailable. In sparse family breakdowns, `N/A` means no counter row was emitted; it can represent zero events when the row's calls/hits/misses reconcile, and does not mean a required scope was measured if its availability is explicitly marked unavailable in the coverage notes.

### golden-v1

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 12863 | 502190 | 502152 | 75659 | 1014421 | 78.86 | 13.41 | 765791 | 248630 | 138182 | 0 | 702370 | 14880342 | 0 | 0 | 0 | 603376 | 14045912 | 12869 | 308760 | N/A | N/A | 825361 | 87139 | 225321 | 128 | 47621 | 0 |
| cpi | 12863 | 502190 | 502152 | 75659 | 380614 | 29.59 | 5.03 | 234076 | 146538 | 49336 | 0 | 225266 | 3228483 | 0 | 0 | 0 | 603376 | 14045912 | 38600 | 3343294 | 151318 | 92442 | 825361 | 42338 | 91674 | 128 | 34626 | 0 |
| prototype181 | 12863 | 502190 | 502152 | 75659 | 1058689 | 82.30 | 13.99 | 787760 | 270929 | 142241 | 0 | 746635 | 15380182 | 29608 | 29608 | 3509 | 603376 | 14045912 | 86751 | 1166110 | N/A | N/A | 825361 | 91156 | 233397 | 128 | 51652 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 149532 | 125447 | 24085 | 2372 |
| feature_generation | original | 135007 | 132087 | 2920 | 60298 |
| feature_generation | cpi | 70746 | 57386 | 13360 | 35110 |
| feature_generation | prototype181 | 59351 | 58217 | 1134 | 59109 |
| crf_state_lookup | original | 590014 | 496259 | 93755 | 75535 |
| crf_state_lookup | cpi | 215414 | 137626 | 77788 | 13901 |
| crf_state_lookup | prototype181 | 560406 | 466651 | 93755 | 78411 |
| crf_transition_lookup | original | 112356 | 107117 | 5239 | 1161 |
| crf_transition_lookup | cpi | 9852 | 8736 | 1116 | 187 |
| crf_transition_lookup | prototype181 | 112356 | 107117 | 5239 | 1161 |
| phrase_search | original | 177044 | 30328 | 146716 | 1188 |
| phrase_search | cpi | 84602 | 30328 | 54274 | 138 |
| phrase_search | prototype181 | 177044 | 30328 | 146716 | 1188 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 135007 | 132087 | 2920 | 60298 |
| word_vocab | cpi | 70746 | 57386 | 13360 | 35110 |
| word_vocab | prototype181 | 135010 | 132090 | 2920 | 60298 |
| state_feature | original | 590014 | 496259 | 93755 | 75535 |
| state_feature | cpi | 215414 | 137626 | 77788 | 13901 |
| state_feature | prototype181 | 634279 | 518225 | 116054 | 79594 |
| transition_feature | original | 112356 | 107117 | 5239 | 1161 |
| transition_feature | cpi | 9852 | 8736 | 1116 | 187 |
| transition_feature | prototype181 | 112356 | 107117 | 5239 | 1161 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 151318 | 5063 | 146255 | 1188 |
| address_phrase | cpi | 58876 | 5063 | 53813 | 138 |
| address_phrase | prototype181 | 151318 | 5063 | 146255 | 1188 |
| component_phrase | original | 12863 | 12845 | 18 | N/A |
| component_phrase | cpi | 12863 | 12845 | 18 | N/A |
| component_phrase | prototype181 | 12863 | 12845 | 18 | N/A |
| postal_phrase | original | 12863 | 12420 | 443 | N/A |
| postal_phrase | cpi | 12863 | 12420 | 443 | N/A |
| postal_phrase | prototype181 | 12863 | 12420 | 443 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### novel

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 12863 | 494008 | 493986 | 73763 | 1018690 | 79.20 | 13.81 | 753237 | 265453 | 127471 | 0 | 713995 | 15117855 | 0 | 0 | 0 | 601405 | 13538252 | 12881 | 308856 | N/A | N/A | 786211 | 81338 | 208809 | 128 | 44710 | 0 |
| cpi | 12863 | 494008 | 493986 | 73763 | 411689 | 32.01 | 5.58 | 250962 | 160727 | 44519 | 0 | 259623 | 3915838 | 0 | 0 | 0 | 601405 | 13538252 | 38604 | 3179417 | 147526 | 88069 | 786211 | 38171 | 82690 | 128 | 31996 | 0 |
| prototype181 | 12863 | 494008 | 493986 | 73763 | 1060903 | 82.48 | 14.38 | 774154 | 286749 | 130869 | 0 | 756194 | 15583717 | 27815 | 27815 | 3105 | 601405 | 13538252 | 82901 | 1109300 | N/A | N/A | 786211 | 84712 | 215581 | 128 | 48430 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 143777 | 118732 | 25045 | 1675 |
| feature_generation | original | 131443 | 122586 | 8857 | 58211 |
| feature_generation | cpi | 66883 | 55370 | 11513 | 31560 |
| feature_generation | prototype181 | 57694 | 52586 | 5108 | 57342 |
| crf_state_lookup | original | 602989 | 496541 | 106448 | 67234 |
| crf_state_lookup | cpi | 246564 | 154563 | 92001 | 12238 |
| crf_state_lookup | prototype181 | 575174 | 468726 | 106448 | 69826 |
| crf_transition_lookup | original | 111006 | 104864 | 6142 | 1266 |
| crf_transition_lookup | cpi | 13059 | 11783 | 1276 | 519 |
| crf_transition_lookup | prototype181 | 111006 | 104864 | 6142 | 1266 |
| phrase_search | original | 173252 | 29246 | 144006 | 760 |
| phrase_search | cpi | 85183 | 29246 | 55937 | 202 |
| phrase_search | prototype181 | 173252 | 29246 | 144006 | 760 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 131443 | 122586 | 8857 | 58211 |
| word_vocab | cpi | 66883 | 55370 | 11513 | 31560 |
| word_vocab | prototype181 | 131457 | 122600 | 8857 | 58211 |
| state_feature | original | 602989 | 496541 | 106448 | 67234 |
| state_feature | cpi | 246564 | 154563 | 92001 | 12238 |
| state_feature | prototype181 | 645188 | 517444 | 127744 | 70632 |
| transition_feature | original | 111006 | 104864 | 6142 | 1266 |
| transition_feature | cpi | 13059 | 11783 | 1276 | 519 |
| transition_feature | prototype181 | 111006 | 104864 | 6142 | 1266 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 147526 | 4610 | 142916 | 760 |
| address_phrase | cpi | 59457 | 4610 | 54847 | 202 |
| address_phrase | prototype181 | 147526 | 4610 | 142916 | 760 |
| component_phrase | original | 12863 | 12845 | 18 | N/A |
| component_phrase | cpi | 12863 | 12845 | 18 | N/A |
| component_phrase | prototype181 | 12863 | 12845 | 18 | N/A |
| postal_phrase | original | 12863 | 11791 | 1072 | N/A |
| postal_phrase | cpi | 12863 | 11791 | 1072 | N/A |
| postal_phrase | prototype181 | 12863 | 11791 | 1072 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### osm-v3

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 11405 | 388815 | 388899 | 61157 | 802562 | 70.37 | 13.12 | 631497 | 171065 | 107740 | 0 | 546890 | 11433102 | 0 | 0 | 0 | 495762 | 11882365 | 11405 | 273720 | N/A | N/A | 638818 | 68679 | 176419 | 128 | 28815 | 0 |
| cpi | 11405 | 388815 | 388899 | 61157 | 284345 | 24.93 | 4.65 | 192890 | 91455 | 35931 | 0 | 160261 | 2155964 | 0 | 0 | 0 | 495762 | 11882365 | 34226 | 2650861 | 122314 | 76294 | 638818 | 31696 | 67627 | 128 | 18610 | 0 |
| prototype181 | 11405 | 388815 | 388899 | 61157 | 838206 | 73.49 | 13.71 | 653519 | 184687 | 110312 | 0 | 582534 | 11832810 | 25312 | 25312 | 2381 | 495762 | 11882365 | 72371 | 974400 | N/A | N/A | 638818 | 71247 | 181559 | 128 | 31380 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 122113 | 108290 | 13823 | 974 |
| feature_generation | original | 110548 | 110243 | 305 | 49783 |
| feature_generation | cpi | 55254 | 46809 | 8445 | 26876 |
| feature_generation | prototype181 | 49391 | 49287 | 104 | 49296 |
| crf_state_lookup | original | 458398 | 409873 | 48525 | 57088 |
| crf_state_lookup | cpi | 154764 | 115151 | 39613 | 9016 |
| crf_state_lookup | prototype181 | 433086 | 384561 | 48525 | 59173 |
| crf_transition_lookup | original | 88492 | 85889 | 2603 | 485 |
| crf_transition_lookup | cpi | 5497 | 5438 | 59 | 25 |
| crf_transition_lookup | prototype181 | 88492 | 85889 | 2603 | 485 |
| phrase_search | original | 145124 | 25492 | 119632 | 384 |
| phrase_search | cpi | 68830 | 25492 | 43338 | 14 |
| phrase_search | prototype181 | 145124 | 25492 | 119632 | 384 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 110548 | 110243 | 305 | 49783 |
| word_vocab | cpi | 55254 | 46809 | 8445 | 26876 |
| word_vocab | prototype181 | 110548 | 110243 | 305 | 49783 |
| state_feature | original | 458398 | 409873 | 48525 | 57088 |
| state_feature | cpi | 154764 | 115151 | 39613 | 9016 |
| state_feature | prototype181 | 494042 | 431895 | 62147 | 59660 |
| transition_feature | original | 88492 | 85889 | 2603 | 485 |
| transition_feature | cpi | 5497 | 5438 | 59 | 25 |
| transition_feature | prototype181 | 88492 | 85889 | 2603 | 485 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 122314 | 3166 | 119148 | 384 |
| address_phrase | cpi | 46020 | 3166 | 42854 | 14 |
| address_phrase | prototype181 | 122314 | 3166 | 119148 | 384 |
| component_phrase | original | 11405 | 11405 | N/A | N/A |
| component_phrase | cpi | 11405 | 11405 | N/A | N/A |
| component_phrase | prototype181 | 11405 | 11405 | N/A | N/A |
| postal_phrase | original | 11405 | 10921 | 484 | N/A |
| postal_phrase | cpi | 11405 | 10921 | 484 | N/A |
| postal_phrase | prototype181 | 11405 | 10921 | 484 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v4

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 10293 | 343765 | 343789 | 53135 | 700851 | 68.09 | 13.19 | 549755 | 151096 | 87275 | 0 | 478440 | 10010533 | 0 | 0 | 0 | 431911 | 10371138 | 10293 | 247032 | N/A | N/A | 562747 | 55448 | 142723 | 128 | 24346 | 0 |
| cpi | 10293 | 343765 | 343789 | 53135 | 249789 | 24.27 | 4.70 | 169392 | 80397 | 29902 | 0 | 141330 | 1926397 | 0 | 0 | 0 | 431911 | 10371138 | 30890 | 2342343 | 106270 | 66988 | 562747 | 26791 | 56693 | 128 | 13806 | 0 |
| prototype181 | 10293 | 343765 | 343789 | 53135 | 731463 | 71.06 | 13.77 | 567128 | 164335 | 89049 | 0 | 509052 | 10357924 | 22290 | 22290 | 1664 | 431911 | 10371138 | 63204 | 860577 | N/A | N/A | 562747 | 57222 | 146271 | 128 | 26134 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 106037 | 92565 | 13472 | 608 |
| feature_generation | original | 95555 | 95118 | 437 | 42655 |
| feature_generation | cpi | 48591 | 42413 | 6178 | 23191 |
| feature_generation | prototype181 | 42420 | 42216 | 204 | 42351 |
| crf_state_lookup | original | 400760 | 356503 | 44257 | 43971 |
| crf_state_lookup | cpi | 136978 | 99937 | 37041 | 6680 |
| crf_state_lookup | prototype181 | 378470 | 334213 | 44257 | 45441 |
| crf_transition_lookup | original | 77680 | 75408 | 2272 | 391 |
| crf_transition_lookup | cpi | 4352 | 4316 | 36 | 7 |
| crf_transition_lookup | prototype181 | 77680 | 75408 | 2272 | 391 |
| phrase_search | original | 126856 | 22726 | 104130 | 258 |
| phrase_search | cpi | 59868 | 22726 | 37142 | 24 |
| phrase_search | prototype181 | 126856 | 22726 | 104130 | 258 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 95555 | 95118 | 437 | 42655 |
| word_vocab | cpi | 48591 | 42413 | 6178 | 23191 |
| word_vocab | prototype181 | 95555 | 95118 | 437 | 42655 |
| state_feature | original | 400760 | 356503 | 44257 | 43971 |
| state_feature | cpi | 136978 | 99937 | 37041 | 6680 |
| state_feature | prototype181 | 431372 | 373876 | 57496 | 45745 |
| transition_feature | original | 77680 | 75408 | 2272 | 391 |
| transition_feature | cpi | 4352 | 4316 | 36 | 7 |
| transition_feature | prototype181 | 77680 | 75408 | 2272 | 391 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 106270 | 2842 | 103428 | 258 |
| address_phrase | cpi | 39282 | 2842 | 36440 | 24 |
| address_phrase | prototype181 | 106270 | 2842 | 103428 | 258 |
| component_phrase | original | 10293 | 10293 | N/A | N/A |
| component_phrase | cpi | 10293 | 10293 | N/A | N/A |
| component_phrase | prototype181 | 10293 | 10293 | N/A | N/A |
| postal_phrase | original | 10293 | 9591 | 702 | N/A |
| postal_phrase | cpi | 10293 | 9591 | 702 | N/A |
| postal_phrase | prototype181 | 10293 | 9591 | 702 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v5

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 8794 | 300364 | 300362 | 47445 | 625169 | 71.09 | 13.18 | 486524 | 138645 | 78026 | 0 | 427671 | 8828100 | 0 | 0 | 0 | 382090 | 9104258 | 8794 | 211056 | N/A | N/A | 493174 | 47721 | 125747 | 128 | 18822 | 0 |
| cpi | 8794 | 300364 | 300362 | 47445 | 225227 | 25.61 | 4.75 | 148913 | 76314 | 26738 | 0 | 127399 | 1733602 | 0 | 0 | 0 | 382090 | 9104258 | 26393 | 2006923 | 94890 | 57990 | 493174 | 23623 | 50361 | 128 | 10885 | 0 |
| prototype181 | 8794 | 300364 | 300362 | 47445 | 652569 | 74.21 | 13.75 | 501815 | 150754 | 80145 | 0 | 455071 | 9139947 | 19742 | 19742 | 1962 | 382090 | 9104258 | 55942 | 753004 | N/A | N/A | 493174 | 49831 | 129976 | 128 | 21535 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 94587 | 82175 | 12412 | 1260 |
| feature_generation | original | 85020 | 84470 | 550 | 38096 |
| feature_generation | cpi | 43340 | 38479 | 4861 | 20643 |
| feature_generation | prototype181 | 37575 | 37328 | 247 | 37466 |
| crf_state_lookup | original | 357373 | 313802 | 43571 | 38526 |
| crf_state_lookup | cpi | 123930 | 87053 | 36877 | 6003 |
| crf_state_lookup | prototype181 | 337631 | 294060 | 43571 | 40015 |
| crf_transition_lookup | original | 70298 | 68288 | 2010 | 924 |
| crf_transition_lookup | cpi | 3469 | 3417 | 52 | 18 |
| crf_transition_lookup | prototype181 | 70298 | 68288 | 2010 | 924 |
| phrase_search | original | 112478 | 19964 | 92514 | 480 |
| phrase_search | cpi | 54488 | 19964 | 34524 | 74 |
| phrase_search | prototype181 | 112478 | 19964 | 92514 | 480 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 85020 | 84470 | 550 | 38096 |
| word_vocab | cpi | 43340 | 38479 | 4861 | 20643 |
| word_vocab | prototype181 | 85020 | 84470 | 550 | 38096 |
| state_feature | original | 357373 | 313802 | 43571 | 38526 |
| state_feature | cpi | 123930 | 87053 | 36877 | 6003 |
| state_feature | prototype181 | 384773 | 329093 | 55680 | 40645 |
| transition_feature | original | 70298 | 68288 | 2010 | 924 |
| transition_feature | cpi | 3469 | 3417 | 52 | 18 |
| transition_feature | prototype181 | 70298 | 68288 | 2010 | 924 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 94890 | 2756 | 92134 | 480 |
| address_phrase | cpi | 36900 | 2756 | 34144 | 74 |
| address_phrase | prototype181 | 94890 | 2756 | 92134 | 480 |
| component_phrase | original | 8794 | 8794 | N/A | N/A |
| component_phrase | cpi | 8794 | 8794 | N/A | N/A |
| component_phrase | prototype181 | 8794 | 8794 | N/A | N/A |
| postal_phrase | original | 8794 | 8414 | 380 | N/A |
| postal_phrase | cpi | 8794 | 8414 | 380 | N/A |
| postal_phrase | prototype181 | 8794 | 8414 | 380 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v6

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 9450 | 318936 | 318909 | 49312 | 647255 | 68.49 | 13.13 | 506281 | 140974 | 74139 | 0 | 442942 | 9095468 | 0 | 0 | 0 | 398146 | 9603710 | 9450 | 226800 | N/A | N/A | 522554 | 47555 | 121694 | 128 | 17999 | 0 |
| cpi | 9450 | 318936 | 318909 | 49312 | 233883 | 24.75 | 4.74 | 158352 | 75531 | 28635 | 0 | 129686 | 1701706 | 0 | 0 | 0 | 398146 | 9603710 | 28361 | 2141436 | 98624 | 61381 | 522554 | 25932 | 54567 | 128 | 13020 | 0 |
| prototype181 | 9450 | 318936 | 318909 | 49312 | 676695 | 71.61 | 13.72 | 522502 | 154193 | 76301 | 0 | 472382 | 9431364 | 19703 | 19703 | 2034 | 398146 | 9603710 | 58600 | 795424 | N/A | N/A | 522554 | 49713 | 126014 | 128 | 20494 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 98455 | 85067 | 13388 | 802 |
| feature_generation | original | 86789 | 86497 | 292 | 37814 |
| feature_generation | cpi | 48054 | 42787 | 5267 | 23218 |
| feature_generation | prototype181 | 37477 | 37354 | 123 | 37413 |
| crf_state_lookup | original | 369146 | 326444 | 42702 | 35431 |
| crf_state_lookup | cpi | 126049 | 90330 | 35719 | 5389 |
| crf_state_lookup | prototype181 | 349443 | 306741 | 42702 | 37192 |
| crf_transition_lookup | original | 73796 | 71702 | 2094 | 614 |
| crf_transition_lookup | cpi | 3637 | 3597 | 40 | 0 |
| crf_transition_lookup | prototype181 | 73796 | 71702 | 2094 | 614 |
| phrase_search | original | 117524 | 21638 | 95886 | 280 |
| phrase_search | cpi | 56143 | 21638 | 34505 | 28 |
| phrase_search | prototype181 | 117524 | 21638 | 95886 | 280 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 86789 | 86497 | 292 | 37814 |
| word_vocab | cpi | 48054 | 42787 | 5267 | 23218 |
| word_vocab | prototype181 | 86789 | 86497 | 292 | 37814 |
| state_feature | original | 369146 | 326444 | 42702 | 35431 |
| state_feature | cpi | 126049 | 90330 | 35719 | 5389 |
| state_feature | prototype181 | 398586 | 342665 | 55921 | 37593 |
| transition_feature | original | 73796 | 71702 | 2094 | 614 |
| transition_feature | cpi | 3637 | 3597 | 40 | N/A |
| transition_feature | prototype181 | 73796 | 71702 | 2094 | 614 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 98624 | 3146 | 95478 | 280 |
| address_phrase | cpi | 37243 | 3146 | 34097 | 28 |
| address_phrase | prototype181 | 98624 | 3146 | 95478 | 280 |
| component_phrase | original | 9450 | 9450 | N/A | N/A |
| component_phrase | cpi | 9450 | 9450 | N/A | N/A |
| component_phrase | prototype181 | 9450 | 9450 | N/A | N/A |
| postal_phrase | original | 9450 | 9042 | 408 | N/A |
| postal_phrase | cpi | 9450 | 9042 | 408 | N/A |
| postal_phrase | prototype181 | 9450 | 9042 | 408 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v7

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 8973 | 300566 | 300558 | 47256 | 621217 | 69.23 | 13.15 | 484787 | 136430 | 77344 | 0 | 424067 | 8785955 | 0 | 0 | 0 | 380686 | 9121561 | 8973 | 215352 | N/A | N/A | 489734 | 50014 | 127358 | 128 | 21206 | 0 |
| cpi | 8973 | 300566 | 300558 | 47256 | 226080 | 25.20 | 4.78 | 149640 | 76440 | 27510 | 0 | 128277 | 1712311 | 0 | 0 | 0 | 380686 | 9121561 | 26930 | 2013962 | 94512 | 58342 | 489734 | 24380 | 51890 | 128 | 10923 | 0 |
| prototype181 | 8973 | 300566 | 300558 | 47256 | 648793 | 72.31 | 13.73 | 500221 | 148572 | 79113 | 0 | 451643 | 9096208 | 19264 | 19264 | 1680 | 380686 | 9121561 | 55818 | 752212 | N/A | N/A | 489734 | 51781 | 130894 | 128 | 23163 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 94096 | 81538 | 12558 | 666 |
| feature_generation | original | 84692 | 83979 | 713 | 37681 |
| feature_generation | cpi | 43687 | 36906 | 6781 | 20780 |
| feature_generation | prototype181 | 37436 | 37139 | 297 | 37348 |
| crf_state_lookup | original | 354605 | 312977 | 41628 | 38997 |
| crf_state_lookup | cpi | 123497 | 87900 | 35597 | 6698 |
| crf_state_lookup | prototype181 | 335341 | 293713 | 41628 | 40433 |
| crf_transition_lookup | original | 69462 | 67709 | 1753 | 480 |
| crf_transition_lookup | cpi | 4780 | 4712 | 68 | 4 |
| crf_transition_lookup | prototype181 | 69462 | 67709 | 1753 | 480 |
| phrase_search | original | 112458 | 20122 | 92336 | 186 |
| phrase_search | cpi | 54116 | 20122 | 33994 | 28 |
| phrase_search | prototype181 | 112458 | 20122 | 92336 | 186 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 84692 | 83979 | 713 | 37681 |
| word_vocab | cpi | 43687 | 36906 | 6781 | 20780 |
| word_vocab | prototype181 | 84692 | 83979 | 713 | 37681 |
| state_feature | original | 354605 | 312977 | 41628 | 38997 |
| state_feature | cpi | 123497 | 87900 | 35597 | 6698 |
| state_feature | prototype181 | 382181 | 328411 | 53770 | 40766 |
| transition_feature | original | 69462 | 67709 | 1753 | 480 |
| transition_feature | cpi | 4780 | 4712 | 68 | 4 |
| transition_feature | prototype181 | 69462 | 67709 | 1753 | 480 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 94512 | 2587 | 91925 | 186 |
| address_phrase | cpi | 36170 | 2587 | 33583 | 28 |
| address_phrase | prototype181 | 94512 | 2587 | 91925 | 186 |
| component_phrase | original | 8973 | 8973 | N/A | N/A |
| component_phrase | cpi | 8973 | 8973 | N/A | N/A |
| component_phrase | prototype181 | 8973 | 8973 | N/A | N/A |
| postal_phrase | original | 8973 | 8562 | 411 | N/A |
| postal_phrase | cpi | 8973 | 8562 | 411 | N/A |
| postal_phrase | prototype181 | 8973 | 8562 | 411 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v8

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 9981 | 337214 | 337224 | 51647 | 678533 | 67.98 | 13.14 | 527855 | 150678 | 86894 | 0 | 462327 | 9693521 | 0 | 0 | 0 | 413723 | 9774340 | 9981 | 239544 | N/A | N/A | 549665 | 56085 | 142979 | 128 | 23306 | 0 |
| cpi | 9981 | 337214 | 337224 | 51647 | 244366 | 24.48 | 4.73 | 160258 | 84108 | 28974 | 0 | 137413 | 1890176 | 0 | 0 | 0 | 413723 | 9774340 | 29954 | 2244524 | 103294 | 63524 | 549665 | 26173 | 55147 | 128 | 11829 | 0 |
| prototype181 | 9981 | 337214 | 337224 | 51647 | 708739 | 71.01 | 13.72 | 545114 | 163625 | 89097 | 0 | 492533 | 10038027 | 20817 | 20817 | 2084 | 413723 | 9774340 | 61013 | 831600 | N/A | N/A | 549665 | 58275 | 147372 | 128 | 25526 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 102670 | 89099 | 13571 | 702 |
| feature_generation | original | 92950 | 92013 | 937 | 41584 |
| feature_generation | cpi | 47221 | 39411 | 7810 | 22622 |
| feature_generation | prototype181 | 41303 | 40990 | 313 | 41233 |
| crf_state_lookup | original | 387917 | 341768 | 46149 | 44635 |
| crf_state_lookup | cpi | 132185 | 93540 | 38645 | 6294 |
| crf_state_lookup | prototype181 | 367100 | 320951 | 46149 | 46487 |
| crf_transition_lookup | original | 74410 | 71955 | 2455 | 439 |
| crf_transition_lookup | cpi | 5228 | 5188 | 40 | 0 |
| crf_transition_lookup | prototype181 | 74410 | 71955 | 2455 | 439 |
| phrase_search | original | 123256 | 22119 | 101137 | 236 |
| phrase_search | cpi | 59732 | 22119 | 37613 | 58 |
| phrase_search | prototype181 | 123256 | 22119 | 101137 | 236 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 92950 | 92013 | 937 | 41584 |
| word_vocab | cpi | 47221 | 39411 | 7810 | 22622 |
| word_vocab | prototype181 | 92950 | 92013 | 937 | 41584 |
| state_feature | original | 387917 | 341768 | 46149 | 44635 |
| state_feature | cpi | 132185 | 93540 | 38645 | 6294 |
| state_feature | prototype181 | 418123 | 359027 | 59096 | 46838 |
| transition_feature | original | 74410 | 71955 | 2455 | 439 |
| transition_feature | cpi | 5228 | 5188 | 40 | N/A |
| transition_feature | prototype181 | 74410 | 71955 | 2455 | 439 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 103294 | 2714 | 100580 | 236 |
| address_phrase | cpi | 39770 | 2714 | 37056 | 58 |
| address_phrase | prototype181 | 103294 | 2714 | 100580 | 236 |
| component_phrase | original | 9981 | 9981 | N/A | N/A |
| component_phrase | cpi | 9981 | 9981 | N/A | N/A |
| component_phrase | prototype181 | 9981 | 9981 | N/A | N/A |
| postal_phrase | original | 9981 | 9424 | 557 | N/A |
| postal_phrase | cpi | 9981 | 9424 | 557 | N/A |
| postal_phrase | prototype181 | 9981 | 9424 | 557 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v9

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 9480 | 334139 | 334135 | 51025 | 670497 | 70.73 | 13.14 | 520456 | 150041 | 87017 | 0 | 457948 | 9572689 | 0 | 0 | 0 | 407611 | 9639591 | 9480 | 227520 | N/A | N/A | 540320 | 55656 | 142673 | 128 | 24039 | 0 |
| cpi | 9480 | 334139 | 334135 | 51025 | 243595 | 25.70 | 4.77 | 158429 | 85166 | 29505 | 0 | 138552 | 1901370 | 0 | 0 | 0 | 407611 | 9639591 | 28451 | 2200931 | 102050 | 62758 | 540320 | 26312 | 55817 | 128 | 13278 | 0 |
| prototype181 | 9480 | 334139 | 334135 | 51025 | 700652 | 73.91 | 13.73 | 536065 | 164587 | 88991 | 0 | 488103 | 9914440 | 20332 | 20332 | 1891 | 407611 | 9639591 | 59976 | 814271 | N/A | N/A | 540320 | 57625 | 146616 | 128 | 26580 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 101512 | 86428 | 15084 | 760 |
| feature_generation | original | 91539 | 90647 | 892 | 40776 |
| feature_generation | cpi | 46791 | 38829 | 7962 | 22372 |
| feature_generation | prototype181 | 40514 | 40160 | 354 | 40396 |
| crf_state_lookup | original | 383970 | 335780 | 48190 | 45645 |
| crf_state_lookup | cpi | 133266 | 92256 | 41010 | 7124 |
| crf_state_lookup | prototype181 | 363638 | 315448 | 48190 | 47239 |
| crf_transition_lookup | original | 73978 | 71831 | 2147 | 426 |
| crf_transition_lookup | cpi | 5286 | 5146 | 140 | 1 |
| crf_transition_lookup | prototype181 | 73978 | 71831 | 2147 | 426 |
| phrase_search | original | 121010 | 22198 | 98812 | 170 |
| phrase_search | cpi | 58252 | 22198 | 36054 | 8 |
| phrase_search | prototype181 | 121010 | 22198 | 98812 | 170 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 91539 | 90647 | 892 | 40776 |
| word_vocab | cpi | 46791 | 38829 | 7962 | 22372 |
| word_vocab | prototype181 | 91539 | 90647 | 892 | 40776 |
| state_feature | original | 383970 | 335780 | 48190 | 45645 |
| state_feature | cpi | 133266 | 92256 | 41010 | 7124 |
| state_feature | prototype181 | 414125 | 351389 | 62736 | 47619 |
| transition_feature | original | 73978 | 71831 | 2147 | 426 |
| transition_feature | cpi | 5286 | 5146 | 140 | 1 |
| transition_feature | prototype181 | 73978 | 71831 | 2147 | 426 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 102050 | 3655 | 98395 | 170 |
| address_phrase | cpi | 39292 | 3655 | 35637 | 8 |
| address_phrase | prototype181 | 102050 | 3655 | 98395 | 170 |
| component_phrase | original | 9480 | 9478 | 2 | N/A |
| component_phrase | cpi | 9480 | 9478 | 2 | N/A |
| component_phrase | prototype181 | 9480 | 9478 | 2 | N/A |
| postal_phrase | original | 9480 | 9065 | 415 | N/A |
| postal_phrase | cpi | 9480 | 9065 | 415 | N/A |
| postal_phrase | prototype181 | 9480 | 9065 | 415 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v10

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 8567 | 289952 | 289949 | 43685 | 575901 | 67.22 | 13.18 | 443647 | 132254 | 66639 | 0 | 392553 | 8079860 | 0 | 0 | 0 | 345962 | 8144275 | 8567 | 205608 | N/A | N/A | 476527 | 42464 | 109103 | 128 | 17418 | 0 |
| cpi | 8567 | 289952 | 289949 | 43685 | 210288 | 24.55 | 4.81 | 136938 | 73350 | 23991 | 0 | 119534 | 1656354 | 0 | 0 | 0 | 345962 | 8144275 | 25712 | 1959186 | 87370 | 54077 | 476527 | 21764 | 45755 | 128 | 12016 | 0 |
| prototype181 | 8567 | 289952 | 289949 | 43685 | 601117 | 70.17 | 13.76 | 457054 | 144063 | 68527 | 0 | 417769 | 8370150 | 18145 | 18145 | 1843 | 345962 | 8144275 | 51937 | 714946 | N/A | N/A | 476527 | 44352 | 112879 | 128 | 19263 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 87046 | 74913 | 12133 | 270 |
| feature_generation | original | 78844 | 78268 | 576 | 35187 |
| feature_generation | cpi | 40327 | 35713 | 4614 | 19313 |
| feature_generation | prototype181 | 35159 | 34907 | 252 | 35052 |
| crf_state_lookup | original | 327889 | 283539 | 44350 | 31149 |
| crf_state_lookup | cpi | 116230 | 78736 | 37494 | 4653 |
| crf_state_lookup | prototype181 | 309744 | 265394 | 44350 | 32902 |
| crf_transition_lookup | original | 64664 | 62423 | 2241 | 193 |
| crf_transition_lookup | cpi | 3304 | 3072 | 232 | 9 |
| crf_transition_lookup | prototype181 | 64664 | 62423 | 2241 | 193 |
| phrase_search | original | 104504 | 19417 | 85087 | 110 |
| phrase_search | cpi | 50427 | 19417 | 31010 | 16 |
| phrase_search | prototype181 | 104504 | 19417 | 85087 | 110 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 78844 | 78268 | 576 | 35187 |
| word_vocab | cpi | 40327 | 35713 | 4614 | 19313 |
| word_vocab | prototype181 | 78844 | 78268 | 576 | 35187 |
| state_feature | original | 327889 | 283539 | 44350 | 31149 |
| state_feature | cpi | 116230 | 78736 | 37494 | 4653 |
| state_feature | prototype181 | 353105 | 296946 | 56159 | 33037 |
| transition_feature | original | 64664 | 62423 | 2241 | 193 |
| transition_feature | cpi | 3304 | 3072 | 232 | 9 |
| transition_feature | prototype181 | 64664 | 62423 | 2241 | 193 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 87370 | 2706 | 84664 | 110 |
| address_phrase | cpi | 33293 | 2706 | 30587 | 16 |
| address_phrase | prototype181 | 87370 | 2706 | 84664 | 110 |
| component_phrase | original | 8567 | 8567 | N/A | N/A |
| component_phrase | cpi | 8567 | 8567 | N/A | N/A |
| component_phrase | prototype181 | 8567 | 8567 | N/A | N/A |
| postal_phrase | original | 8567 | 8144 | 423 | N/A |
| postal_phrase | cpi | 8567 | 8144 | 423 | N/A |
| postal_phrase | prototype181 | 8567 | 8144 | 423 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v11

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 7882 | 267309 | 267310 | 41716 | 551957 | 70.03 | 13.23 | 424591 | 127366 | 68001 | 0 | 376374 | 7722983 | 0 | 0 | 0 | 331582 | 7802176 | 7882 | 189168 | N/A | N/A | 437783 | 42600 | 110601 | 128 | 18004 | 0 |
| cpi | 7882 | 267309 | 267310 | 41716 | 199962 | 25.37 | 4.79 | 129711 | 70251 | 22612 | 0 | 115696 | 1592340 | 0 | 0 | 0 | 331582 | 7802176 | 23657 | 1809205 | 83432 | 52665 | 437783 | 20457 | 43069 | 128 | 10180 | 0 |
| prototype181 | 7882 | 267309 | 267310 | 41716 | 575943 | 73.07 | 13.81 | 437343 | 138600 | 69838 | 0 | 400360 | 7989224 | 17436 | 17436 | 1740 | 331582 | 7802176 | 49313 | 665562 | N/A | N/A | 437783 | 44435 | 114273 | 128 | 19861 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 83138 | 71610 | 11528 | 520 |
| feature_generation | original | 76387 | 75757 | 630 | 34797 |
| feature_generation | cpi | 37735 | 33091 | 4644 | 18008 |
| feature_generation | prototype181 | 34671 | 34335 | 336 | 34537 |
| crf_state_lookup | original | 314722 | 272069 | 42653 | 32698 |
| crf_state_lookup | cpi | 112304 | 76058 | 36246 | 4569 |
| crf_state_lookup | prototype181 | 297286 | 254633 | 42653 | 34275 |
| crf_transition_lookup | original | 61652 | 59513 | 2139 | 304 |
| crf_transition_lookup | cpi | 3392 | 3310 | 82 | 3 |
| crf_transition_lookup | prototype181 | 61652 | 59513 | 2139 | 304 |
| phrase_search | original | 99196 | 17252 | 81944 | 202 |
| phrase_search | cpi | 46531 | 17252 | 29279 | 32 |
| phrase_search | prototype181 | 99196 | 17252 | 81944 | 202 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 76387 | 75757 | 630 | 34797 |
| word_vocab | cpi | 37735 | 33091 | 4644 | 18008 |
| word_vocab | prototype181 | 76387 | 75757 | 630 | 34797 |
| state_feature | original | 314722 | 272069 | 42653 | 32698 |
| state_feature | cpi | 112304 | 76058 | 36246 | 4569 |
| state_feature | prototype181 | 338708 | 284821 | 53887 | 34535 |
| transition_feature | original | 61652 | 59513 | 2139 | 304 |
| transition_feature | cpi | 3392 | 3310 | 82 | 3 |
| transition_feature | prototype181 | 61652 | 59513 | 2139 | 304 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 83432 | 1928 | 81504 | 202 |
| address_phrase | cpi | 30767 | 1928 | 28839 | 32 |
| address_phrase | prototype181 | 83432 | 1928 | 81504 | 202 |
| component_phrase | original | 7882 | 7882 | N/A | N/A |
| component_phrase | cpi | 7882 | 7882 | N/A | N/A |
| component_phrase | prototype181 | 7882 | 7882 | N/A | N/A |
| postal_phrase | original | 7882 | 7442 | 440 | N/A |
| postal_phrase | cpi | 7882 | 7442 | 440 | N/A |
| postal_phrase | prototype181 | 7882 | 7442 | 440 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v12

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 8273 | 284768 | 284767 | 42457 | 559160 | 67.59 | 13.17 | 434579 | 124581 | 65705 | 0 | 380772 | 7889098 | 0 | 0 | 0 | 339217 | 8073100 | 8273 | 198552 | N/A | N/A | 466674 | 43170 | 108875 | 128 | 15928 | 0 |
| cpi | 8273 | 284768 | 284767 | 42457 | 200472 | 24.23 | 4.72 | 131755 | 68717 | 23193 | 0 | 111462 | 1511280 | 0 | 0 | 0 | 339217 | 8073100 | 24830 | 1957223 | 84914 | 51680 | 466674 | 21113 | 44306 | 128 | 10314 | 0 |
| prototype181 | 8273 | 284768 | 284767 | 42457 | 583651 | 70.55 | 13.75 | 447076 | 136575 | 67463 | 0 | 405263 | 8172357 | 17713 | 17713 | 1686 | 339217 | 8073100 | 50482 | 695579 | N/A | N/A | 466674 | 44928 | 112391 | 128 | 18220 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 84661 | 72414 | 12247 | 286 |
| feature_generation | original | 76928 | 76412 | 516 | 34504 |
| feature_generation | cpi | 39230 | 34488 | 4742 | 18567 |
| feature_generation | prototype181 | 34471 | 34208 | 263 | 34361 |
| crf_state_lookup | original | 317370 | 277802 | 39568 | 30882 |
| crf_state_lookup | cpi | 107968 | 74876 | 33092 | 4616 |
| crf_state_lookup | prototype181 | 299657 | 260089 | 39568 | 32497 |
| crf_transition_lookup | original | 63402 | 61415 | 1987 | 173 |
| crf_transition_lookup | cpi | 3494 | 3441 | 53 | 1 |
| crf_transition_lookup | prototype181 | 63402 | 61415 | 1987 | 173 |
| phrase_search | original | 101460 | 18950 | 82510 | 146 |
| phrase_search | cpi | 49780 | 18950 | 30830 | 9 |
| phrase_search | prototype181 | 101460 | 18950 | 82510 | 146 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 76928 | 76412 | 516 | 34504 |
| word_vocab | cpi | 39230 | 34488 | 4742 | 18567 |
| word_vocab | prototype181 | 76928 | 76412 | 516 | 34504 |
| state_feature | original | 317370 | 277802 | 39568 | 30882 |
| state_feature | cpi | 107968 | 74876 | 33092 | 4616 |
| state_feature | prototype181 | 341861 | 290299 | 51562 | 32640 |
| transition_feature | original | 63402 | 61415 | 1987 | 173 |
| transition_feature | cpi | 3494 | 3441 | 53 | 1 |
| transition_feature | prototype181 | 63402 | 61415 | 1987 | 173 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 84914 | 2785 | 82129 | 146 |
| address_phrase | cpi | 33234 | 2785 | 30449 | 9 |
| address_phrase | prototype181 | 84914 | 2785 | 82129 | 146 |
| component_phrase | original | 8273 | 8273 | N/A | N/A |
| component_phrase | cpi | 8273 | 8273 | N/A | N/A |
| component_phrase | prototype181 | 8273 | 8273 | N/A | N/A |
| postal_phrase | original | 8273 | 7892 | 381 | N/A |
| postal_phrase | cpi | 8273 | 7892 | 381 | N/A |
| postal_phrase | prototype181 | 8273 | 7892 | 381 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

### unseen-v13

| Variant | Addresses | Input bytes | Normalized bytes | Tokens | Lookup calls | Calls/address | Calls/token | Hits | Misses | Repeats | Answers reused | Feature keys | Key bytes | Direct IDs offered | Direct IDs consumed | Direct ID fallbacks | Rows | Weights | Alloc calls | Alloc bytes | Filter checks | Searches avoided | Output bytes | Distinct repeat groups | Repeat observations | Retained examples | Omitted groups | Overflow addresses |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | 9716 | 320599 | 320598 | 50560 | 662121 | 68.15 | 13.10 | 511947 | 150174 | 83158 | 0 | 450513 | 9207323 | 0 | 0 | 0 | 399471 | 9533883 | 9716 | 233184 | N/A | N/A | 529622 | 54169 | 137327 | 128 | 22105 | 0 |
| cpi | 9716 | 320599 | 320598 | 50560 | 240234 | 24.73 | 4.75 | 155315 | 84919 | 28190 | 0 | 135530 | 1788994 | 0 | 0 | 0 | 399471 | 9533883 | 29159 | 2182248 | 101120 | 61473 | 529622 | 25651 | 53841 | 128 | 12951 | 0 |
| prototype181 | 9716 | 320599 | 320598 | 50560 | 691084 | 71.13 | 13.67 | 526968 | 164116 | 85382 | 0 | 479476 | 9532296 | 20963 | 20963 | 2082 | 399471 | 9533883 | 59648 | 803678 | N/A | N/A | 529622 | 56393 | 141775 | 128 | 24492 | 0 |

#### By Stage lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| normalization | original | 0 | 0 | 0 | 0 |
| normalization | cpi | 0 | 0 | 0 | 0 |
| normalization | prototype181 | 0 | 0 | 0 | 0 |
| tokenization | original | 0 | 0 | 0 | 0 |
| tokenization | cpi | 0 | 0 | 0 | 0 |
| tokenization | prototype181 | 0 | 0 | 0 | 0 |
| context_fill | original | 0 | 0 | 0 | 0 |
| context_fill | cpi | 0 | 0 | 0 | 0 |
| context_fill | prototype181 | 100486 | 85910 | 14576 | 744 |
| feature_generation | original | 91056 | 90116 | 940 | 40694 |
| feature_generation | cpi | 45625 | 38671 | 6954 | 21952 |
| feature_generation | prototype181 | 40496 | 40190 | 306 | 40322 |
| crf_state_lookup | original | 377007 | 328258 | 48749 | 41690 |
| crf_state_lookup | cpi | 130707 | 89538 | 41169 | 6214 |
| crf_state_lookup | prototype181 | 356044 | 307295 | 48749 | 43542 |
| crf_transition_lookup | original | 73506 | 71213 | 2293 | 482 |
| crf_transition_lookup | cpi | 4823 | 4746 | 77 | 4 |
| crf_transition_lookup | prototype181 | 73506 | 71213 | 2293 | 482 |
| phrase_search | original | 120552 | 22360 | 98192 | 292 |
| phrase_search | cpi | 59079 | 22360 | 36719 | 20 |
| phrase_search | prototype181 | 120552 | 22360 | 98192 | 292 |
| response_build | original | 0 | 0 | 0 | 0 |
| response_build | cpi | 0 | 0 | 0 | 0 |
| response_build | prototype181 | 0 | 0 | 0 | 0 |
| unclassified | original | 0 | 0 | 0 | 0 |
| unclassified | cpi | 0 | 0 | 0 | 0 |
| unclassified | prototype181 | 0 | 0 | 0 | 0 |

#### By Family lookup totals

| Dimension | Variant | Calls | Hits | Misses | Repeats |
| --- | --- | --- | --- | --- | --- |
| word_vocab | original | 91056 | 90116 | 940 | 40694 |
| word_vocab | cpi | 45625 | 38671 | 6954 | 21952 |
| word_vocab | prototype181 | 91056 | 90116 | 940 | 40694 |
| state_feature | original | 377007 | 328258 | 48749 | 41690 |
| state_feature | cpi | 130707 | 89538 | 41169 | 6214 |
| state_feature | prototype181 | 405970 | 343279 | 62691 | 43914 |
| transition_feature | original | 73506 | 71213 | 2293 | 482 |
| transition_feature | cpi | 4823 | 4746 | 77 | 4 |
| transition_feature | prototype181 | 73506 | 71213 | 2293 | 482 |
| phrase_dictionary | original | N/A | N/A | N/A | N/A |
| phrase_dictionary | cpi | N/A | N/A | N/A | N/A |
| phrase_dictionary | prototype181 | N/A | N/A | N/A | N/A |
| feature_key | original | N/A | N/A | N/A | N/A |
| feature_key | cpi | N/A | N/A | N/A | N/A |
| feature_key | prototype181 | N/A | N/A | N/A | N/A |
| transition_feature_key | original | N/A | N/A | N/A | N/A |
| transition_feature_key | cpi | N/A | N/A | N/A | N/A |
| transition_feature_key | prototype181 | N/A | N/A | N/A | N/A |
| model_row | original | N/A | N/A | N/A | N/A |
| model_row | cpi | N/A | N/A | N/A | N/A |
| model_row | prototype181 | N/A | N/A | N/A | N/A |
| direct_id | original | N/A | N/A | N/A | N/A |
| direct_id | cpi | N/A | N/A | N/A | N/A |
| direct_id | prototype181 | N/A | N/A | N/A | N/A |
| address_phrase | original | 101120 | 3533 | 97587 | 292 |
| address_phrase | cpi | 39647 | 3533 | 36114 | 20 |
| address_phrase | prototype181 | 101120 | 3533 | 97587 | 292 |
| component_phrase | original | 9716 | 9716 | N/A | N/A |
| component_phrase | cpi | 9716 | 9716 | N/A | N/A |
| component_phrase | prototype181 | 9716 | 9716 | N/A | N/A |
| postal_phrase | original | 9716 | 9111 | 605 | N/A |
| postal_phrase | cpi | 9716 | 9111 | 605 | N/A |
| postal_phrase | prototype181 | 9716 | 9111 | 605 | N/A |
| other_trie | original | N/A | N/A | N/A | N/A |
| other_trie | cpi | N/A | N/A | N/A | N/A |
| other_trie | prototype181 | N/A | N/A | N/A | N/A |
| address | original | N/A | N/A | N/A | N/A |
| address | cpi | N/A | N/A | N/A | N/A |
| address | prototype181 | N/A | N/A | N/A | N/A |
| parser_token | original | N/A | N/A | N/A | N/A |
| parser_token | cpi | N/A | N/A | N/A | N/A |
| parser_token | prototype181 | N/A | N/A | N/A | N/A |
| parser_output | original | N/A | N/A | N/A | N/A |
| parser_output | cpi | N/A | N/A | N/A | N/A |
| parser_output | prototype181 | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | original | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | cpi | N/A | N/A | N/A | N/A |
| prefix_suffix_flag | prototype181 | N/A | N/A | N/A | N/A |
| unclassified | original | N/A | N/A | N/A | N/A |
| unclassified | cpi | N/A | N/A | N/A | N/A |
| unclassified | prototype181 | N/A | N/A | N/A | N/A |
| diagnostic_tracker | original | N/A | N/A | N/A | N/A |
| diagnostic_tracker | cpi | N/A | N/A | N/A | N/A |
| diagnostic_tracker | prototype181 | N/A | N/A | N/A | N/A |
| parser_alloc | original | N/A | N/A | N/A | N/A |
| parser_alloc | cpi | N/A | N/A | N/A | N/A |
| parser_alloc | prototype181 | N/A | N/A | N/A | N/A |
| crf_alloc | original | N/A | N/A | N/A | N/A |
| crf_alloc | cpi | N/A | N/A | N/A | N/A |
| crf_alloc | prototype181 | N/A | N/A | N/A | N/A |

## Evidence and interpretation

- Output checks are reported by the runner as byte-identical for each release and diagnostic output against the original output; thread evidence and input/output hashes remain in `results.json`.
- Reproduction identity: build records `research/experiments/185-diagnostic-instrumentation/raw/build-20261007-1625-pattern-summary-fix`, artifacts `research/experiments/185-diagnostic-instrumentation/artifacts/build-20261007-1625-pattern-summary-fix`, source/config/input/model hashes in `manifest.json`, and per-dataset output hashes in `results.json`.
- Repeated-pattern summaries are counts from all tracked queries. Any retained pattern rows are first-encountered examples, not top-frequency results; see raw result JSON for their bounded summaries.
- Exact repeat identity is dictionary pointer + operation + key bytes within one run. Exported pattern rows merge by family + operation + key bytes and cannot establish cross-dictionary sharing.
- Feature-key counts/bytes describe scorer-boundary strings, not all feature constructors or intermediate text. cpi `EXP_CP_IDS` and `EXP_ROLE_IDS` first-use precomputed model-ID request/consumption counts are not instrumented and remain unavailable; do not classify them as repeated-answer reuse.
- Allocation metrics cover direct malloc/calloc/realloc sites in `address_parser.c` and `crf.c` during parsing only; they are not total heap growth or RSS. Hash probes, phrase-trie edges, complete key construction, and general text-copy bytes are unavailable.
- `filter_checks` and `searches_avoided` apply only to cpi's enabled `EXP_PS_FLAGS` path; original and prototype181 have no matching filter path.
- Model row/weight counts describe scoring workload and remain distinct from repeated lookup counts; equal totals do not prove the work is irreducible. This report does not infer savings or performance improvement.
- Full source and metric boundary inventory: [COVERAGE.md](../../COVERAGE.md). Reproduction records, calibration and output evidence are retained in this experiment directory.
