# Experiment 187 — CPI answer reuse and consumer contract

Status: source audit and trace-classification contract; no parser/source changes
and no new parser run in this document. The associated 185 diagnostic run is
evidence about counts, not a proof that any removed lookup can be removed from
the model semantics.

## Scope and provenance

This audit follows the production CPI implementation recorded by experiment 119
through its copied, instrumented counterpart in experiment 185. The reference
build flags are the exact `cc: clang | lib: ...` line in
`research/final/119-cpi/build_hashes.txt` (SHA-256 of that manifest:
`50bcadabf56801896ebf8f13a64924553965c031969c95e53a9361d25ac0dc40`). The
selected CPI additions are listed in `research/final/119-cpi/switches.txt`.
The full production source identities for those files are listed in
`research/final/119-cpi/build_hashes.txt`; in particular, address parser SHA
`a3db12f9...f18536` and CRF SHA `cc001bb7...048860`.

Experiment 185's copied diagnostic CPI source is at
`research/experiments/185-diagnostic-instrumentation/source/cpi/`. Its
`crf_context.c` is byte-identical to 119 (`9a8fd433...cdaf6`), while
`address_parser.c` (`6a699a28...a492d`) and `crf.c`
(`fe020aa7...3762fd`) differ because the instrumentation adds diagnostics.
Therefore, use 119 for production behavior/flag identity and 185 copies for
mapping the instrumented trace sites. Source line references below use the
185 copied paths unless explicitly marked 119. Do not interpret trace counters
as complete logical-request counts: 185 explicitly documents missed requests
on `EXP_CP_IDS` and `EXP_ROLE_IDS` paths (`185-diagnostic-instrumentation/README.md`,
“Selected golden-v1 counters” and “Known limitations”).

## Core identity rule

A reusable answer is equivalent only when all of these agree:

1. The same live dictionary/model generation and the same dictionary object.
2. The same operation and key representation: exact key bytes and length, or
   the exact typed/structured key actually used by the table.
3. The same result domain and miss semantics (for example, frequency versus
   feature ID; state versus transition; phrase match versus feature lookup).
4. A consumer that accepts that answer at this callsite, with the original
   occurrence multiplicity, token/role/position, and scoring contribution
   preserved.
5. Storage that remains valid through every consumer and is reset or
   generation-invalidated before the dictionary, parse context, or address
   data changes.

Equal query text is only a necessary hint. It does not make two requests
equivalent across dictionaries, feature namespaces, arities, token positions,
or consuming weight rows. A repeated row ID can still contribute more than
once: CRF scoring iterates each emitted feature ID and adds every row weight to
its target label/transition score (`crf.c:749-778` in the 185 copied source;
see the state and transition loops). Any reuse/de-duplication must retain the
emission count and score additions unless an independently proven algebraic
reduction preserves the final ordered parse output.

## Concrete call/data-flow map

| Flow or stored value | Producer / source location | Immediate consumer | Safe validity boundary | Audit result |
|---|---|---|---|---|
| Vocabulary frequency and vocabulary slot | `word_vocab_frequency` (`address_parser.c:496-508`); CPI memo fill via `fd_find_slot` and copies the wide count (`address_parser.c:1700-1738`) | Feature construction reads frequency and canonical slot while `address_parser_features` is called for each token (`:1828+`, `:1860-1872`) | One address and the active vocabulary dictionary; memo resets at first token (`idx == 0`) and only accepts the exact normalized-token pointer (`:1720-1734`, `:1860-1864`) | Same token lookup may be skipped if pointer identity, bytes, length, dictionary generation and both returned fields match. Different normalized tokens with equal bytes are not automatically covered by this memo; trace must establish identity. Frequency answer is not a state/transition answer. |
| Canonical vocabulary ID for unary routed features | `ap_canon_part` plus `ap_ik_emit1` (`address_parser.c:1764-1805`) | Emits a typed integer key or pre-resolved role feature ID into the state or transition side queue; CRF lookup consumer follows in `crf.c:507-578` | Current vocabulary + integerized feature dictionaries; canonical IDs are dictionary slot IDs, not globally stable IDs | May replace a fresh encoding/lookup only when dictionary generation, role, key form and target table match. Cannot reuse across state/trans tables or a changed vocab/model. |
| Canonical pair ID for bigram routed feature | `ap_ik_emit2` (`address_parser.c:1807-1826`) canonicalizes first part as non-last (strip trailing `|`) and second as last, then calls `fdi_key(name,c1,c2)` | Side queue routes to one of the state/trans `fdi_lookup` tables (`crf.c:507-578`) | Active dictionary generation and exact ordered pair of canonical IDs | Reversing arguments, changing name/arity, or treating the separator rule as irrelevant changes the request. Pair is not interchangeable with two unigram answers. |
| Word/phrase feature result | `word_or_phrase_at_index` and memo wrapper (`address_parser.c:1294-1414`) | Nearby-token feature generation calls `AP_WORD_OR_PHRASE` (`address_parser.c:2270-2295`) | Phrase arrays, normalized token arrays, model phrases and address-scoped memo; reset at `idx == 0` (`:1387-1414`, `:1867-1873`) | Memo is source-proven to address-scoped and only for `long_context=false`. Plain strings point into normalized context; phrase result text is copied into per-entry `char_array` because scratch buffers are overwritten. Do not carry its phrase pointer across parse/reset. The result type and phrase metadata must travel with bytes. |
| Joined phrase bytes / length | `ap_phrase_memo` (`address_parser.c:1498-1513`) stores joined address-dictionary or component phrase text by phrase index and generation | Phrase feature emitters consume bytes plus explicit length (`address_parser.c:1943-2070`) | `ap_pm_gen` advances at token index 0 (`:1876`); per-index buffer remains stable only during that generation. Allocation failure falls back to mutable scratch buffer. | Safe within the current parse only when generation is unchanged and the owned memo path actually succeeded. The fallback pointer is scratch and must not be retained. Phrase index alone is insufficient across context fills. |
| Component phrase precomputed feature IDs (`EXP_CP_IDS`) | `cp_build` walks the component phrase trie and computes emitted state feature IDs from phrase type/name and the state dictionary (`address_parser.c:1516-1608`); this is model-setup precomputation, not a runtime duplicate-query hit | At a component phrase match, `address_parser_features` checks matched data ID and exact joined-string length, then pushes the sequence (`address_parser.c:1996-2013`); CRF consumes it from state queue (`crf.c:507-578`) | Same phrase trie/type table and same state feature dictionary generation as build; requires `cp_ready`, valid phrase data index, supported nonzero length <=255, and feature sequence order/count | Eligible path replaces construction+lookup with a precomputed ordered ID sequence. It is not a global string cache, does not prove all phrase answers are identical, and fallback `ap_cp_emit` remains for unsupported cases. The sequence must preserve every feature-ID occurrence and ordering. |
| Role IDs (`EXP_ROLE_IDS`) | `crf_int_keys_enable` makes `[vocab slot][role]` table after creating integer split dictionaries (`crf.c:422-476`) | `ap_ik_emit1` selects the role index and puts a high-bit resolved-ID marker in the state/trans queue (`address_parser.c:1791-1805`); CRF copies that ID to its answer vector (`crf.c:519-578`) | Same vocabulary slot assignment, state/trans integer dictionary generation, and role mapping; built once after dictionaries are loaded | Setup-time answer table, not evidence of a repeated request at runtime. Roles are distinct even for same token. `CRF_NO_FEATURE` remains a meaningful miss result; high-bit path must not reinterpret it as a valid feature. |
| Constant feature IDs (`EXP_CONST_IDS`) | `crf_int_keys_enable` looks up `bias` and `postcode have context` in state dictionary once (`crf.c:454-457`) | `AP_CONST` emits the ID only while integer-key accumulation is active (`address_parser.c:1750-1754`); CRF resolves the tagged value (`crf.c:519-578`) | Same state dictionary generation; enabled only after integer dictionary replacement succeeded | These are value-less constant features, not duplicate address query answers. Preserve their emission multiplicity (e.g. bias per relevant token if that is what the feature callback emits). |
| Prefix/suffix negative flags | `ap_ftt_build` derives flags by searching every vocabulary word against prefix/suffix dictionaries (`address_parser.c:258-277`); `EXP_PS_FLAGS` uses a flag only when raw token bytes exactly equal normalized-admin token bytes (`:1175-1200`) | Its answer is a stored `NULL_PHRASE` in that address context’s prefix/suffix arrays; feature selection reads those arrays (`:1294+`) | Same phrase dictionaries, vocabulary, tokenizer, exact raw/admin byte equality and model generation. Context arrays cleared/refilled per address (`:999-1008`, `:1098+`) | Only a negative search result is reused. A failed guard takes the ordinary search. A flag from another token, normalization, or dictionary generation is not reusable. |
| First-token trie table (`EXP_FTT`) | `ap_ftt_build` walks model vocabulary into component phrase trie and stores the first-token walk result by vocab slot (`address_parser.c:258-277`) | `address_parser_context_fill` maps normalized-admin token to vocab slot and searches component trie (`:1135-1170`); feature path uses same slot for role prefetch (`:1150-1163`) | Same phrase trie/vocabulary slots; per-context fill sets `trie_ftt_slot` and clears it after phrase search (`:1093-1170`) | Structural search acceleration from model data. Slot/walk result is not a general phrase answer and cannot be carried across model generations or token normalization changes. |
| Integer-key queue and placeholder | `crf_ik_push` appends the typed key to a state or transition side queue and an empty string placeholder in matching feature array (`crf.c:400-417`) | `crf_dict_lookup_array` consumes placeholders in order, distinguishes direct IDs from `fdi_lookup` requests, and validates the count (`crf.c:497-605`) | Single active feature-accumulation pass/address; the two queues are separate and reset at start of direct scoring (`crf.c:647-675`) | Positional protocol: equal counts and order are invariants. Reordering, collapsing duplicate placeholders, or sharing state and transition queue is unsafe without changing the consumer and proving identical output. |
| Feature ID and sparse weight contribution | Text or typed lookup yields an ID/`CRF_NO_FEATURE` in the per-feature answer vector (`crf.c:497-605`) | State lookup walks `weights->indptr/indices/data`; transition lookup separately walks `state_trans_weights` and adds into all L×L score cells (`crf.c:730-778`) | Current model weights and CRF context for this address/token position | Same feature ID may be reused as a lookup answer, but its score addition is per emitted occurrence and has token/role/label context. Eliminating additions based on duplicate IDs is not source-proven safe. |
| Per-address parse objects (`EXP_REUSE`) | Static `rs_tokens`, tokenized string, token-label, label, component arrays are allocated lazily (`address_parser.c:2470-2483`) | `address_parser_parse` clears/refills them; block response copies arrays before return (`:2514-2560`, `:2638-2715`) | Reused parser process, one call at a time, with clear-before-fill and response block ownership | This is storage reuse, not answer reuse. It imposes single-thread/non-reentrant mutable global state and depends on `EXP_RESPONSE_BLOCK`; it says nothing about cross-address query equivalence. |

## Generation, mutability, setup, and teardown

### Source-proven stability during one parse

The trace format's numeric generation fields are null, but that does not make
every within-parse generation unknowable. For the specified synchronous,
single-thread parse invocation, the source call graph establishes a narrower
stability boundary:

- `libpostal_parse_address` forwards directly to `address_parser_parse`
  (`source/cpi/libpostal.c:279-281`); `get_address_parser` only returns the
  existing parser pointer (`source/cpi/address_parser.c:78-80`). It does not
  set up or reload a parser.
- Setup builds/loads the vocab, CRF dictionaries, phrase tries and context
  before a parse (`source/cpi/address_parser.c:330-445`). Teardown is a
  separate API path (`source/cpi/libpostal.c:467-490`) and destroys those
  objects (`source/cpi/address_parser.c:454-488`, `source/cpi/crf.c:1250-1275`).
- The parse body (`source/cpi/address_parser.c:2485-2739`) performs normalize,
  tokenize, context fill, prediction, and response construction; it contains
  no setup/teardown call. Runtime feature-dictionary probes take
  `const feature_dict_t *` and only read slots/arena
  (`source/cpi/feature_dict.h:125-190`); `fdi_lookup` likewise reads its
  table (`:164-170`). Runtime CRF resolution reads state/transition tables
  (`source/cpi/crf.c:489-605`) and model weight rows, writing per-address score
  context (`:749-778`). Phrase/vocab trie search fills output phrase/context
  arrays, while trie mutation routines (`trie_add*`, `trie_set_data_node`) are
  builder/setup operations, not calls on this parse path
  (`source/cpi/trie.c:401-721`).

Therefore, when all of these scope conditions are met—one parse invocation,
single-threaded synchronous execution, no signal/plugin callback that mutates
the model, and exact same dictionary pointer—the source establishes that its
contents remain stable between query and consumer. Instrumentation may label
this **within-parse source-stable, epoch-unversioned**: this is a control-flow
and API-constness proof, not an emitted generation number. It supports a
within-parse repeated-query comparison when answer shape and consumer are also
proven. It does not prove pointer identity across addresses, parser teardown/
setup, model reloads, concurrent API use, or lifetime of an owned/caller
buffer. Those broader scopes remain unknown until versioned or tested.

1. `address_parser_setup` loads model dictionaries, converts CRF tries to feature
   dictionaries, enables integer-key tables, builds vocabulary/phrase lookup
   tables, then creates a parser context (`address_parser.c:330-445`). These
   operations define a model generation. `crf_int_keys_enable` replaces the
   state and transition dictionaries and frees the original dictionaries
   unless the verify build retains them (`crf.c:422-455`); any pointer-keyed
   memo must be invalidated across this replacement.
2. `address_parser_context_fill` clears and repopulates normalized token arrays,
   phrase arrays and phrase-membership arrays on every address
   (`address_parser.c:999-1008`, `:1098-1101`, `:1200+`). Phrase indices,
   memberships and normalized-string pointers are therefore context-generation
   identities, not stable keys across inputs.
3. `address_parser_features` executes per token and starts vocabulary/word
   memo generations at `idx == 0` (`address_parser.c:1860-1873`). `ap_pm_gen`
   also increments at `idx == 0` (`:1875+`). The CRF direct scorer resets the
   state/trans typed-key queues at scoring start, enables accumulation, visits
   tokens in ascending order, then disables accumulation before dictionary
   lookup and score accumulation (`crf.c:647-716`). This bounds memo and queue
   validity to one score pass.
4. `EXP_REUSE` holds mutable parse scratch in process-global statics; parser
   teardown destroys parser-owned structures (`address_parser.c:440-477`) but
   the static scratch and memo buffers are not tied to one parser object. The
   reviewed source does not establish safe concurrent/reentrant calls or safe
   reuse across teardown/setup. A redesigned candidate must either prove those
   public lifecycle states safe or retain exact fallback/reset behavior. Do
   not assume setup generation is stable merely because a process remains
   alive.
5. `crf_role_ids`, `crf_const_ids`, component phrase ID pools, FTT tables, and
   feature-dictionary pointers are mutable setup-time globals/statics
   (`crf.c:383-399,422-476`; `address_parser.c:245-277,1516-1608`). Their
   teardown/rebuild invalidation is not fully established by the source audit
   alone. Unless a lifecycle trace proves it, classify cross-setup reuse as
   **unproven**.

## Query-equivalence classes and non-equivalences

- **Equal text, different domain:** `word` in `vocab_dict` returns a frequency
  and possibly a canonical slot; `word|<value>` in state or transition
  dictionaries returns a feature ID. Same bytes never bridges these results.
- **Equal text, different dictionary:** state and transition keys use separate
  dictionaries and weight tables (`crf.c:730-778`); answer equality must be
  established per table/generation.
- **Same value, different role:** `word`, `prev word`, `next word`, `first word`
  and two pair roles are distinct dictionary names (`crf.c:418-420`). Role
  changes the feature ID even when the normalized token is identical.
- **Same token string, different position:** first/last/nonlast pair position
  affects feature encoding. `ap_canon_part` strips a trailing separator for
  nonlast components (`address_parser.c:1764-1789`). Neighbor selection and
  token-indexed phrases also vary by position.
- **Same feature ID, repeated model contribution:** state/transition rows can
  be added multiple times. Reusing an ID may avoid lookup, but de-duplicating
  feature occurrences changes score accumulation unless separately proven
  observationally equivalent.
- **Phrase match versus phrase feature:** `phrase_t` match data and membership
  select the feature-emission branch, while feature dictionary lookup answers
  map emitted key encodings to feature IDs. They are separate operations and
  lifetimes.
- **Same phrase bytes, different phrase source/metadata:** street/address
  dictionary phrase, component phrase and postal phrase arrays are distinct;
  type/data index and membership must accompany the bytes.
- **Same feature key on cache hit versus copied answer:** a batch-local exact
  duplicate can reuse a just-found answer only within the same table batch and
  retains separate answer-vector positions. A CPI setup table (role/constant/
  component phrase) is a precomputed answer source, not a runtime query repeat.

## What the 185 measurements establish

The 185 report says golden-v1 cpi has 380,614 instrumented lookup calls,
49,336 repeats, and 3.84 repeats/address; original has 1,014,421 calls and
138,182 repeats. These are measured counters over the sites included by that
instrumentation, not total feature requests or saved searches. The report
explicitly notes that `EXP_CP_IDS` and `EXP_ROLE_IDS` precomputed-ID paths are
not fully counted as direct logical requests (`research/experiments/185-diagnostic-instrumentation/README.md`).
The 185 coverage document lists source-site coverage and omissions; use those
omissions to bound every workload interpretation. A count reduction alone does
not say whether the request was duplicated, moved to setup, served by a role
table, or removed together with a feature emission.

## Final3 golden preservation trace: what it establishes

The final3 golden-preservation run processed 12,863 golden-v1 inputs. Oracle,
release-control, and diagnostic outputs were byte-identical (SHA-256
`95aad44a8c95c2dcd64a51cd34dca31cf73ca85150a171d3f87b1295d37940ec`); their
thread samples were all one. The frozen ordered-component comparison also
matched all 12,863 rows, while its negative control produced one mismatch. The
run is a correctness/observability capture, not a speed or candidate-memory
benchmark (`results/golden-preservation-build-187-final3/results.json`).

The analyzed trace is complete for 12,863 addresses and contains 989,085 query
events and 738,383 linked consumer events. It records 59,107 existing
`pointer_matched_token_memo` word-vocabulary answers. Setup/precomputed IDs are
a separate lane: 371,204 state and 102,504 transition ID events; they are not
physical query repeats and must not be counted as cache hits or saved searches.
Among physical searches, the analyzer reports these repeated exact-identity
upper bounds:

| Family | Later physical searches in repeated groups | Interpretation limit |
|---|---:|---|
| Word vocabulary | 36,299 | Includes untyped misses; source proves missing vocabulary words return frequency zero, but the trace does not encode that returned zero as the query answer. |
| State feature | 13,901 | Consumer mapping is partial; preserve each feature-ID occurrence and score contribution. |
| Transition feature | 187 | Consumer mapping is partial; preserve each feature-ID occurrence and score contribution. |
| Address phrase | 138 | Phrase payloads/effects are not captured, so these are only repeated physical searches, not proven reusable answers. |
| Component phrase / postal phrase | 0 | No typed phrase answer was established. |

These are upper bounds on later physical searches among repeated dictionary,
operation, family, and key identities. They are not measured avoidable work,
benefit, or speedup. The current parser already serves some vocabulary requests
from its token memo, and these observed memo answers must not be attributed as
improvement from a proposed redesign. The full trace report marks answer or
consumer proof incomplete for many groups and consumer coverage incomplete;
positive matches therefore remain partial evidence.

Two instrumentation limits matter when reading the aggregate flow tables:

- A false `answer_matches_source` or consumer-match flag can mean “untyped,” not
  “different answer.” For word vocabulary, `word_vocab_frequency` initializes
  `count` to zero and explicitly leaves it zero on `fd_lookup` miss
  (`source/cpi/address_parser.c:496-520`). `ap_vm_get` initializes memo count to
  zero on a missing `fd_find_slot`, and `ap_vocab_freq` returns that count
  (`:1731-1764`). The trace preserves `hit=false` but writes a null lookup
  payload, while the immediate consumer receives typed `vocab_frequency=0`.
  Thus miss-versus-zero rows are untyped comparisons, not evidence of semantic
  disagreement. A future trace should record the lookup status and returned
  scalar answer as separate fields.
- `producer_site_id` is not a reliable callsite for address-phrase searches in
  this capture. The wrappers set stage, family, and operation but do not set or
  restore the global site (`address_parser.c:523-539`); `diag.c:120-121`
  serializes that global, and the precomputed-ID path can leave it set to
  `crf.integer_queue.precomputed_id` (`diag.c:282-289`). The phrase repeat counts
  remain counts of captured identities, but their site attribution must not be
  used to infer a producer/consumer path.

The final4 source revision addresses both trace limits: word-vocabulary lookup
rows carry the returned `vocab_frequency` scalar (including zero) independently
of the `hit` flag, and phrase/vocabulary/precomputed sites are set around their
instrumented call and restored afterward. This is source-reviewed only here;
final4 trace results must replace, not retroactively rewrite, the final3
measurement above.

The final4 golden-v1 preservation run now confirms those fixes on the same
12,863 inputs. The trace source-manifest SHA-256 is
`016337d972512ba098591796044fa42323d3afcf534c9457b1e030bb63f6d33e`, and
before/after source-manifest hashes match. Oracle, release-control, diagnostic,
and frozen ordered-component checks remain byte/semantically identical, with
the negative control still producing one mismatch. All 59,107 vocabulary
token-memo flows now compare as equal, and vocabulary consumers—including
misses returned as zero—compare equal. Address-phrase repeat groups now carry
the explicit `address_parser.address_prefix` or
`address_parser.address_suffix` site labels instead of the stale precomputed-ID
label. Phrase result payloads are still incomplete; site correction does not
make their answers reusable. The repeated-search upper bounds by family are
unchanged from final3 and remain upper bounds, not measured benefits. These are
golden-v1-only final4 results; the other 12 configured workloads are separate
and must be reported from their own completed captures
(`results/golden-preservation-build-187-final4/golden-v1/analysis.json`).

The analyzer keeps only one address's query/consumer rows while streaming. Its
per-address rows can report lower bounds for simultaneously live repeated key
bytes and numeric payload bytes; those omit map slots, pointers, allocator
overhead, and other state. Neither estimate is a measured cache footprint or
memory comparison against the original.

## Final4 reviewed all13 evidence

The reviewed reanalysis at
`results/reviewed-final4-v2/overall-analysis.json` supersedes the final3
analysis and the golden-only interim counts above for current all13 conclusions.
It reanalyzes retained final4 traces and invokes no parser. All 13 configured
workloads are complete, with 128,540/128,540 declared address summaries and a
single source-manifest SHA-256 (`016337d972512ba098591796044fa42323d3afcf534c9457b1e030bb63f6d33e`).
The coordinator audit records 8,903,320 query events and 6,767,700 consumer
events. Captured oracle, release-control, and diagnostic outputs are
byte-identical per workload; all recorded thread samples are one. These verify
preservation and instrumentation coverage for these inputs, not runtime or
memory acceptance of a candidate.

Aggregate search counts across the separate configured workloads are:

| Family | Physical searches | Later physical searches in repeated exact-identity groups |
|---|---:|---:|
| Address phrase | 529,951 | 651 |
| Component phrase | 128,540 | 0 |
| Postal phrase | 128,540 | 0 |
| State feature | 1,859,856 | 93,395 |
| Transition feature | 70,173 | 778 |
| Word vocabulary | 1,322,284 | 310,066 |
| **Total** | **4,039,344** | **404,890** |

The repeated-search column is an upper bound on potentially avoidable physical
searches (10.02% of captured physical searches), not an estimate of speedup.
Its evidence statuses are 249,635 consumer-link-incomplete, 98,957
negative-answer-or-consumer-proof-incomplete, and 56,298 observed-match but
coverage-partial searches. By family, the state feature repeat evidence is
43,643 negative/proof-incomplete and 49,752 matched at covered sites with
coverage still partial; transition is 24 and 754 respectively. Word
vocabulary is 249,618 consumer-link-incomplete, 54,656 negative/proof-incomplete,
and 5,792 matched at covered sites with coverage partial. Address phrase is 17
consumer-link-incomplete and 634 negative/effects-incomplete. No family has a
globally complete consumer-coverage proof for the repeated groups.

The current instrumentation does establish 546,206 exact typed answers served
by the existing pointer-matched token memo; these are already avoided lookups,
not new redesign savings. It also records 3,373,241 state and 944,529
transition precomputed-ID events. Those 4,317,770 events are setup/precomputed
answer paths, separate from physical searches and repeat groups; they must not
be counted as cache hits. All typed vocabulary flows and consumers compare
exactly, including miss-as-frequency-zero. State and transition linked score
consumers likewise compare exact resolved IDs at observed links, but the
site coverage remains partial. A feature ID's repeated score contribution is
still per occurrence.

Phrase answer reuse remains unproven: hit rows without typed results total
41,491 address-phrase, 128,502 component-phrase, and 121,819 postal-phrase
rows. A phrase predicate's boolean or incomplete payload does not establish
the wrapper's output-array side effects or a reusable answer, including on a
miss. Keep those operations out of a reuse candidate absent typed payload,
effect, and consumer evidence.

Per-address liveness summarizes only repeated-answer intervals for which the
analyzer has typed key/payload sizes and linked consumers, from the first
physical request through the last linked consumer. It is a measured subset,
not a lifetime or footprint estimate for every repeated group or a proposed
cache. Across all 128,540 addresses, the maximum simultaneously live repeated
answers has p50=0, p95=2,
and max=26; key-byte lower bounds are p50=0, p95=66, max=905; numeric-payload
lower bounds are p50=0, p95=8, max=104. The query-to-last-consumer event-span
statistic has p50=0, p95=84, max=486. This is the interval a carried value
would need to survive in the observed linked flow. A first-consumer-to-last-
consumer lag is unavailable and is not inferred. Byte counts omit hash slots,
pointers, allocator/alignment overhead, and cache metadata, so they are lower
bounds rather than a memory comparison.

The next experiment should test a bounded carry through already-existing
per-token storage for one source-reviewed typed scalar/ID result and its actual
consumer, with exact identity, preserved occurrence/score multiplicity, and
the existing lookup as fallback. State has the largest partially linked
feature-ID repeat population (49,752 covered-site matches); transition has
754, so those typed routes are reasonable candidates for a small prototype,
not already-proven removable work. The source call graph proves dictionary/model
reads stable within one synchronous parse after setup, but numeric
dictionary/model generation fields remain null, so cross-address and cross-
setup carrying is not proven. Word vocabulary has the largest repeat upper
bound, but 249,618 repeat searches lack consumer links and the existing token
memo already provides 546,206 observed answers. Close the
`feature_vocab_canon` consumer mapping before proposing another vocabulary
cache. Do not pursue phrase-negative reuse from these data. Any candidate still
needs all-workload byte checks, one-thread validation, and measured memory no
greater than the original before it can be accepted. The broad repeat and
probe counts describe workload shape; they do not determine cache cost or
reject a design without a measured candidate.

These totals are not directly comparable with experiment 185's 380,614
instrumented lookup calls / 49,336 repeats for golden-v1. Experiment 185 has
different site coverage and does not fully count `EXP_CP_IDS` or `EXP_ROLE_IDS`;
187 separates physical searches, answer reuse, and precomputed IDs and includes
12 additional configured workloads. The differing counts do not establish a
parser regression or a directly comparable work reduction.

## Proof conditions for trace classifications

For every pair considered for answer carrying, report these independent facts:

| Predicate | Required evidence | If missing |
|---|---|---|
| Repeat observed | Same address/parse ID and exact request identity, not merely same text | `repeat_unknown` |
| Identity exact | Same dictionary identity + generation, operation/domain, exact key bytes and length (or exact typed key tuple) | `identity_unknown` |
| Answer equal | Same hit/miss and exact typed answer value; for phrases include phrase kind/data/extent; for rows include feature ID | `answer_unknown` |
| Consumer linked | Answer source/query index linked to immediate consumer callsite and downstream semantic use | `consumer_unknown` |
| Lifetime valid | Value remains valid and unchanged through consumer; parse and model generations match | `lifetime_unknown` |
| Multiplicity retained | Every original occurrence still reaches required feature emission and score contribution | `multiplicity_unknown` |
| Reuse benefit real | Avoided physical search/encoding is distinguished from a request served by precomputed/setup data; report storage/setup overhead | `benefit_unknown` |

Only a pair satisfying all seven predicates may be called **demonstrated
within-parse answer reuse candidate**. It is still not a speedup claim: benchmark
the added storage/lookup work and compare output bytes. If a call has no linked
consumer event, call it an observed lookup/repeat only. If generation is null,
do not assert cross-address or cross-setup validity. If a value is a feature ID,
do not suppress its repeated scoring contribution.

Required event fields for the diagnostic analyzer: parse/address identity,
request/query identity, operation and semantic family, dictionary identity and
generation (nullable when unavailable), exact key bytes/length or a collision-
safe digest plus collision-safe equality check, request stage/site, physical
search versus memo/precomputed answer source, hit/miss and typed answer, and
consumer site/role/stage with source query index. Declare absent sites and
counter overflow. Avoid raw user addresses in exported summaries.

## Evidence status and unknowns

- **Source-proven:** setup-time creation of integer dictionaries/role tables;
  exact queue-order consumption; address-scoped word/vocabulary memo resets;
  phrase scratch overwrite and the owned-copy memo; component phrase precompute
  guards and fallback; distinct state/transition score consumers; repeated row
  additions; global mutable scratch under `EXP_REUSE`.
- **Measured in experiment 185:** instrumented counter totals and output parity
  on its declared datasets/builds (see 185 report and retained raw records).
  Those results do not prove cache safety outside each tested run.
- **Not established by this source audit:** full teardown/setup rebuilding of
  every CPI global; concurrent/reentrant behavior of `EXP_REUSE`; total logical
  request counts including precomputed ID emissions; lifetime of every
  diagnostic query-result temporary at every callsite; universal equality of
  parser behavior for arbitrary strings.
- **Inference/risk:** any new address-crossing answer cache risks stale IDs,
  token-context misbinding, score multiplicity changes and memory overhead.
  Exact fallback to the existing operation is the safe design boundary until
  the trace demonstrates the predicates above and output tests validate the
  candidate.

## Reproducible source locations

Paths and line numbers can be checked from the workspace root with:

```sh
rg -n 'EXP_(VOCAB_MEMO|WORD_MEMO|DIRECT_KEYS|INT_KEYS|ROLE_IDS|CP_IDS|FTT|PS_FLAGS|FA_LEN|REUSE)|crf_ik_push|crf_dict_lookup_array|crf_tagger_score_direct|ap_vm_get|ap_wm_reset|ap_phrase_memo|ap_cp_emit|cp_build|address_parser_context_fill|address_parser_features' \
  research/experiments/185-diagnostic-instrumentation/source/cpi/address_parser.c \
  research/experiments/185-diagnostic-instrumentation/source/cpi/crf.c
```

The 119 production flag line and source hashes are preserved separately from
185 diagnostic variants. This audit did not rebuild, execute parser inputs, or
modify either source tree.
