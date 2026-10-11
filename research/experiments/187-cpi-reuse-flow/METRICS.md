# Experiment 187 CPI reuse-flow metrics

Reviewed measurements are generated from the retained final4 traces in [`results/reviewed-final4-v2/overall-analysis.json`](results/reviewed-final4-v2/overall-analysis.json). Per-workload summaries and per-address rows are under the same directory. This is analyzer-only reprocessing; the trace, parser outputs, and collection records are unchanged.

## Scope and validity

- 13 observed workloads, 128,540 input rows total. Country and language are recorded as `unknown`; no country or language strata are inferred.
- All 13 workloads passed exact output comparison between the pinned original oracle, the CPI release control, and the CPI diagnostic build. The golden-v1 frozen component comparison passed 12,863/12,863; its negative control detected the intentional mismatch. Sampled process thread counts were 1.
- The diagnostic trace reports complete address/event counts with no overflow. The source stability proof is address-scoped to one synchronous parse after setup; numeric dictionary/model generations remain null, so cross-address or re-setup reuse is not established.
- Corpus traces preserve query key bytes as reversible hex. Input addresses are identified by hash and length. Consumer mapping is explicitly partial; the trace does not cover every possible query effect or consumer.
- Final4 source manifest SHA-256: `016337d972512ba098591796044fa42323d3afcf534c9457b1e030bb63f6d33e`. Release/diagnostic flag-file SHA-256: `bef574c7789cacb69622291b87efaa1bae04d63bf4f105e50f0868dd1990fb23` / `979d8a614ad8948f3f124673095f83d1e`.

Raw command, timestamp, stdout, stderr, and exit records are preserved in `raw/verification/`. The successful golden and remaining12 command logs are `golden-preservation-command-final4.json` and `remaining12-command-final4.json`; the reviewed reanalysis log is `reanalysis-final4-v2-command.json`. The first interrupted reanalysis remains separately preserved in `results/reviewed-final4/`.

## Event totals

| Event class | Count | Meaning |
|---|---:|---|
| Logical query events | 8,903,320 | Physical searches, observed same-address answer reuse, and precomputed IDs |
| Physical dictionary searches | 4,039,344 | Actual search calls observed by instrumentation |
| Observed answer reuse events | 546,206 | Existing CPI token memo path; all linked source comparisons were exact typed matches |
| Precomputed feature-ID events | 4,317,770 | Role/constant IDs supplied directly, separated from physical searches |
| Linked query-consumer events | 6,767,700 | Captured actual score/materialization consumers; not a complete map of all consumer sites |

The 546,206 word-vocabulary memo events are already-existing CPI reuse. They are not additional savings attributed to a proposed design. The 4,317,770 precomputed IDs are also existing work avoided at runtime lookup sites; they are not duplicate physical queries.

## Repeated physical identities

There were 360,009 repeated exact lookup groups, representing 404,890 later physical requests after each group's first request. This is a **counterfactual upper bound on repeated search calls**, not proven removable work or a performance result. Every group remained partial or unproven under consumer/effect coverage:

| Evidence status | Later searches | Interpretation |
|---|---:|---|
| `consumer-link-incomplete` | 249,635 | At least one repeated request lacks a linked consumer |
| `negative-answer-or-consumer-proof-incomplete` | 98,957 | Miss semantics, result shape, or complete consumer proof is missing |
| `observed-match-covered-sites-coverage-partial` | 56,298 | Captured values match, but instrumentation coverage is partial |
| **Total** | **404,890** | No group meets complete consumer-coverage acceptance |

Operation-specific repeat upper bounds:

| Family | Operation | Later physical requests | Evidence limitation |
|---|---|---:|---|
| `word_vocab` | `feature_vocab_canon` | 304,202 | Generic `instrumented_leaf` site; callsite mapping is unresolved |
| `word_vocab` | `word_vocab_lookup` | 5,864 | Includes `ap_vm_get` and `word_vocab_frequency` routes; incomplete consumer/effect coverage |
| `state_feature` | `feature_hash` | 93,395 | Generic `instrumented_leaf` site; all evidence remains partial |
| `transition_feature` | `feature_hash` | 778 | Generic `instrumented_leaf` site; all evidence remains partial |
| `address_phrase` | `address_prefix` | 315 | Boolean/phrase effects and consumer links do not establish a reusable result |
| `address_phrase` | `address_suffix` | 336 | Boolean/phrase effects and consumer links do not establish a reusable result |

The generic site label is an instrumentation fallback, not a source-level producer location. Repeated phrase booleans are not treated as equivalent phrase-array results. Precomputed IDs have null source dictionary/key identity and are reported separately from dictionary lookup identity.

## Counterfactual lookup and storage costs

If a generic cache were checked before every physical query, it would add up to 4,039,344 probes. For the repeated-identity subset only, the trace contains 360,009 first-seen keys and 4,479,619 key bytes (sum of exact key lengths). Known typed scalar/ID payload widths sum to a 1,172,656-byte one-copy lower bound; 66,845 repeated groups have unknown payload width. These figures exclude hash slots, pointers, alignment, allocator overhead, invalidation and code cost. They are accounting estimates, not a cache prototype or measured memory use.

No candidate implementation was tested. The strongest next design experiment is a bounded direct carry through already-existing per-token storage for a source-reviewed scalar result and its actual consumer, with multiplicity preserved. A global/general-purpose cache is not justified by these counts: its probe work would be much larger than the repeated-search upper bound, and current site/consumer coverage is incomplete.

## Per-address liveness lower bounds

Percentiles use all 128,540 addresses, including zero for addresses with no measured repeated group; p95 is nearest-rank. The span field measures **first physical request through last linked consumer**, the storage-lifetime interval. It does not measure first-consumer-to-last-consumer lag separately.

| Lower-bound metric | p50 | p95 | Max |
|---|---:|---:|---:|
| Simultaneously live repeated answers | 0 | 2 | 26 |
| Live exact key bytes | 0 B | 66 B | 905 B |
| Live typed numeric payload bytes | 0 B | 8 B | 104 B |
| First request to last linked consumer, event rows | 0 | 84 | 486 |

The byte totals omit map/hash slots, pointers, allocator metadata, alignment, and any design-specific copies. They are lower bounds for observed query-to-consumer intervals, not candidate-cache RSS.

## Comparison limits

These counters describe final4 CPI instrumentation and its explicit event schema. They are not directly comparable to experiment 185's aggregate counters: the event definitions and instrumentation coverage differ. The diagnostic build adds tracing; no optimized candidate was benchmarked, so speed, RSS, and optimality remain unmeasured.
