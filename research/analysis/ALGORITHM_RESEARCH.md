# Algorithm research: is each algorithm on the parse path the best one? (research only, nothing run)

Date: 2026-09-30. Companion to `AUDIT_REPORT.md`. The audit asked "is each function necessary and can it be faster". This document asks, for each **algorithm**:
how does it work, what does the literature say is better, is the better version exact (byte-identical output), and is this algorithm actually a bottleneck
(its measured share of build `h`'s time, from the profile in `experiments/008-profile-after-006/raw/` as summarized in `AUDIT_REPORT.md` section 2).
No code was changed and nothing was timed. Every gain is UNVERIFIED until an experiment measures it. Machine: Apple M4 Pro, 24 GB (`sysctl`).

Evidence quality is marked: **[read]** = I read the primary source myself; **[summary]** = from a search-engine summary only, not checked against the source.

## 1. What "fast" means for this program

Runtime = instructions executed + time stalled waiting for memory. Both are runtime; "memory" here never means the footprint.
The measured facts: each address has about 5.9 tokens and triggers about 55 feature lookups (46 state, 9 transition; experiment 001) into tables built
from a ~2.5 GB model. In build `h` the time splits roughly: context fill 24%, feature generation 19%, dictionary lookups 12-15%, weight accumulation 8.7%,
whole-string normalization ~10%, allocation 5.7%, Viterbi 5.2%, response ~3%, tokenizing 2.4%.

The single most relevant published finding is Chen & Manning (2014) **[read]**, about feature-based parsers of exactly libpostal's design:
"we have to concatenate some words, POS tags, or arc labels for generating feature strings, and look them up in a huge table containing several millions of
features. In our experiments, more than 95% of the time is consumed by feature computation during the parsing process." libpostal's CRF parser is that
design (build feature strings, hash them, look them up in a table of ~11.5 M keys, add weight rows). The decoder and the tokenizer are not where the time is.
So the question "is there a better algorithm" matters most for *how features are produced and looked up*, and least for Viterbi.

Note on the model: the Mapzen "Inside libpostal" post **[summary, via the fetch tool's summarizer]** describes an averaged perceptron; the code in this tree loads a CRF
(`crf.c`, `crf_context.c`), so the post is out of date on that point. Its other design claims (phrases compiled to a trie, CLDR transliterators compiled
to a trie, tokenizer "more than 2 million tokens per second") match the code.

## 2. Algorithm by algorithm

| # | Algorithm (where) | How it works now | Best known alternative (source) | Exact? | Share of `h` | Verdict |
|---|---|---|---|---|---:|---|
| A1 | Viterbi decoding (`crf_context.c:567`) | Dynamic programming, O(T·L²), L = 20 labels, T ≈ 6 | CarpeDiem (Esposito & Radicioni, JMLR 2009) **[summary]**: exact, sub-quadratic in L in most cases. Staggered decoding (Kaji et al., ACL 2010) **[read]**: exact, orders of magnitude faster *for large label sets*; the paper itself says plain Viterbi "is efficient enough if L is small" | yes, both exact | 5.2% | **Not the bottleneck.** Already the right algorithm for L = 20. At most the constant-factor items C4 (hoist accessors) and a NEON inner loop. |
| A2 | Feature generation (`address_parser_features`, `feature_array_add`) | ~55 feature *strings* per address, each built by concatenation, then scanned, hashed and compared byte-wise | (a) **Precompute per-word partial scores** (Chen & Manning 2014 **[read]**: precomputing the 10,000 most frequent words made their parser "8 ~ 10 times" faster; their saving was matrix multiplication, so the size of that speedup does NOT transfer here). (b) **Integer feature ids instead of strings**: spaCy maps "all strings ... to integer IDs" **[summary]**; CRFsuite uses a constant quark database for "very fast feature weight retrieval" **[summary]** | yes, see section 3 | 19% (+ part of the 12-15% lookup) | **The main bottleneck, and the place where a different algorithm applies.** Sections 3.1 and 3.2. |
| A3 | Feature dictionary (experiment 006, `feature_dict.h`) | Open addressing, linear probing, 64-bit slots with an 8-bit tag, keys in a separate arena, load factor 0.75 | SwissTable-style group probing with control bytes **[summary]**: avoids touching key memory on misses; linear probing already has ~1 miss per search vs 2 for cuckoo **[summary]**. PtrHash minimal perfect hashing (SEA 2025) **[summary]**: 8-12 ns/key at 10⁹ keys when streamed, but still needs a key check for the 16% misses | yes (same key → same id) | 12-15% | **Table algorithm is near the frontier already** (one slot line per probe, tags filter mismatches). The cost left is the *second and third* cache lines: the arena key compare and the CSR row. Removing those is a layout/keying change (3.2, C1), not a better hash table. |
| A4 | Weight storage (CSR: `indptr`, `indices`, `data`) | A hit reads `indptr[id]`, then `indices[...]` and `data[...]` in separate arrays | Co-locate the row with the key (audit C1). Literature: separate index/value arrays in sparse formats cause extra misses with little locality **[summary]** | yes | 8.7% accumulate + part of lookup | **Real bottleneck, layout fix.** Already audit item C1. |
| A5 | Phrase search over tokens (`trie_search_tokens_from_index`, three tries) | Double-array trie walked byte by byte from every token start, greedy longest match, restarting per token | Aho-Corasick: one pass, cost linear in text length and independent of dictionary size **[summary]**. Double-array tries are already cache-friendly (cedar ~55 ns/key vs 179 ns for `std::unordered_map` in cedar's own benchmark **[summary]**) | Aho-Corasick only if it reproduces libpostal's longest-match/restart state machine exactly | 12.6% own (largest single function) | **Bottleneck; the algorithm is fine, the wasted walks are not.** Most tokens start no phrase, so a 2-byte prefilter (P1) removes most walks. Aho-Corasick (P10) is the asymptotic fix but gains little over a prefilter on 6-token inputs. Your Dataparser uses the same pair: Aho-Corasick for exact gazetteer matching and keyword gating before expensive work. |
| A6 | Whole-string Unicode normalization (`utf8_case`, `utf8proc_map`) | Four decompose passes, two re-encode passes, a lowercase pass, several allocations per non-ASCII string | ASCII fast path (done, 011; PostgreSQL discussion reports CedarDB got 6x from an ASCII fast path **[summary]**). ICU reported ~6.5x faster than utf8proc on one normalization benchmark **[summary]**; a Haskell library beat utf8proc's decompose "several times" **[summary]** | yes, if checked per code point against utf8proc (the oracle) | ~10% (mostly the non-ASCII half) | **Real bottleneck for the floor set.** utf8proc is a slow implementation of a standard algorithm; a single-pass table (audit N5/N6, like Dataparser's generated `variant_fold.hpp` table) is the fix. Swapping in ICU is not safe: its output can differ from utf8proc's. |
| A7 | Transliteration (`transliterate.c`, CLDR rules compiled to a trie) | Rule engine walks the string for each step of `latin-ascii-simple` | Rule interpretation has documented overhead in ICU's transliterator; compiled transforms or plain code are the usual alternative **[summary, weak sources]** | yes, when a rule provably cannot fire | part of the ~10% | Skip steps that cannot fire (audit N1/N2, proven from the rule tables). |
| A8 | Tokenizer (re2c DFA, `scanner.re`) | Generated DFA as direct jumps | re2c aims to be "at least as fast as reasonably optimized hand-coded" lexers **[summary]**; SIMD structural scanning (simdjson) beats state machines for JSON **[summary]** | risky (token *types* must match) | 2.4% | **Not a bottleneck.** Leave it. |
| A9 | Postal-code graph membership (`graph_has_edge`, `graph.c:92`) | Binary search in one CSR row | Eytzinger layout with prefetching (Khuong & Morin **[summary]**): usually fastest for large n; plain sorted order is best for small n | yes | 1-2% | Rows are short, so the win is small. Prefetch the row when the postal phrase is found (P8). |
| A10 | Per-address allocation (malloc/free of dozens of small buffers) | System allocator per string/array | Arena / bump allocation (audit R2). The mimalloc evidence found is about lock contention with many threads **[summary]**, which does not apply to one thread | yes | ~5.7% | Arena and one-block response; no allocator swap needed. |
| A11 | Feature-string hashing (`fd_hash_bytes`) | Custom multiply-xor over the bytes | wyhash/rapidhash ~22 cycles for 1-31 byte keys **[summary]** | yes (hash choice cannot change which key matches) | a few % | Minor. Better: do not hash strings at all (3.2). |
| A12 | Compiler (`-O2`, no LTO/PGO) | Small cross-file functions not inlined | PGO: SPEC-style gains of 1.02-1.09x for Clang **[summary]**; larger real-world claims come from a low-quality page and are not relied on | yes (same source) | whole program | Worth one experiment (X1), trained on the development split only. |

Items from the literature that do **not** apply: speed-accuracy methods such as variable-order CRFs (Vieira, Cotterell & Eisner 2016, 2-6x **[summary]**)
and beam or pruned Viterbi change the model or the output; multi-threading (Dataparser's OpenMP path) is excluded by the single-thread rule.

## 3. The two algorithmic changes the audit did not have

### 3.1 Per-word partial score precomputation (exact)

**Why it is exact (checked in the code build `h` actually runs).** In `research/experiments/011-normalize-fastpaths/candidate/crf.c`
(`crf_tagger_score_direct`), each token's 20 state scores are zeroed (`memset` in `crf_context_set_num_items_lean`) and then every feature's weight row is
added in feature order: `state_scores[indices[col]] += data[col]`. Floating-point addition gives identical bits when the same numbers are added in the same
order. For a token that is in no address, component or postal phrase, `address_parser_features` emits `bias`, then `prefix`/`suffix` (if the token has
a dictionary prefix/suffix), then n-grams (rare words only), then `word` (checked in build `h`'s own source, `011-normalize-fastpaths/candidate/address_parser.c`: `bias` 1417, `prefix` 1435, `suffix` 1453, n-grams 1507, `word` 1521;
phrase and postcode features, lines 1261-1395, come before `bias` only for tokens inside those phrases); all of these depend only on the token.
So the partial score after that block, `((0 + bias) + prefix) + ... + word` per label, can be computed at load and copied in, bit-identical.
The remaining features (`first word`, `prev word`, `next word`, the bigrams, transition features) are then added on top as today.

**Limits found.** (1) Memory: the vocabulary has 3,694,560 keys (experiment 010); 20 doubles each would be ~590 MB, and build `h` has only ~56 MB of
headroom under the baseline's peak (2.557 vs 2.613 GB). It only fits for the top N words ranked by the model's own vocabulary counts (allowed by the
generalization rule), e.g. 65,536 words × 160 B ≈ 10.5 MB. (2) `prefix`/`suffix` use the pre-normalization token, so the table must be keyed on both
forms or used only when they agree. (3) Tokens inside phrases keep the old path. (4) The saving per token is about one cold lookup (`word`) plus its row
and the `bias` row (already hot); smaller than the Chen & Manning analogy suggests.

### 3.2 Integer-keyed feature lookup (symbol interning; exact)

**Idea.** Today `"prev word+word|main street"` is concatenated, scanned for `|`, hashed and compared byte by byte against a key stored in the arena
(a second cold cache line). The token's vocabulary entry is already found once per token, so its id is known. At load, rebuild the part of the dictionary
whose keys have only vocabulary words as values into a table keyed by the integer tuple (name id, word id, word id). The tuple needs about 51 bits (name ~6, two word ids of 22 bits for 3.69 M words) plus the
24-bit feature id and a tag, so it does not fit today's 64-bit slot: use 16-byte slots (still one cache line per probe) or store a tuple hash and verify the tuple.
A lookup then needs no string building, no scan, no string hash, no byte compare, and no arena line.

**Why exact.** Use one predicate on both sides: a key goes to the integer table iff every value part is a vocabulary word; at run time a feature whose
parts are all vocabulary ids is looked up there, anything else (n-grams, phrase strings, prefixes) uses today's string path. The same key string maps to
the same weight row either way; the load-time build can be checked exhaustively against the string table (every key, as in 006).
One more condition: rebuilding tuples splits each model key's value on the separator (a space for two-word features), which is only unambiguous if no vocabulary
word contains that separator. The load-time build must verify this per key; any key that does not split cleanly stays on the string path.

**What it attacks:** feature-string building (`feature_array_add` 8% inclusive in the baseline profile), `fd_split`/`memchr`/hashing, `memcmp` (3.4%),
and one of the cold lines per lookup. The size of the gain depends on the share of lookups that are word-keyed, which has not been measured.

## 4. Conclusions

1. **Most of libpostal's algorithms are already the right ones.** The decoder (Viterbi, L = 20), the tokenizer (re2c DFA), the trie type (double-array)
   and the hash table type (linear probing with tags) are at or near the best known for this input size. Exact improvements exist in the literature
   (CarpeDiem, staggered decoding, Eytzinger, perfect hashing), but they target parts that are 1-5% of the time here. Swapping them in cannot give large gains.
2. **The bottleneck is the feature pipeline, and the published fix is algorithmic:** stop producing and hashing strings (3.2), precompute what depends on
   one word (3.1), compute each shared descriptor once (audit F1/F2), and store weights next to their keys (C1). This matches Chen & Manning's finding
   that feature computation, not decoding, dominates this class of parser.
3. **The second bottleneck is waste in phrase search and normalization,** whose fixes (prefilters, one-pass tables, skipping rule steps that cannot fire)
   are already audit items; the research supports them.
4. **No new speed prediction.** The floor prediction in the previous analysis (about 3.5×, range 3× to 4.5×) stands until 3.1 and 3.2 are measured;
   they are the items most likely to move it up.

## 5. Open measurements (cheap, decide whether 3.1 and 3.2 are worth building)

1. Share of plain (non-phrase) tokens whose word is in the top N vocabulary words by model count, for N = 16K, 64K, 256K (on the development split).
2. Share of the ~55 lookups per address whose value parts are all vocabulary words (the reach of 3.2).
3. Real DRAM latency and memory-level parallelism on this M4 Pro for random reads over a 2.5 GB table (a small pointer-chasing and batched-read probe).
   None of the latency figures found online were for this chip's CPU cores, so none are used here.

## Sources

- Chen & Manning, "A Fast and Accurate Dependency Parser using Neural Networks", EMNLP 2014 [read]: https://aclanthology.org/D14-1082/
- Kaji, Fujiwara, Yoshinaga & Kitsuregawa, "Efficient Staggered Decoding for Sequence Labeling", ACL 2010 [read]: https://aclanthology.org/P10-1050/
- Esposito & Radicioni, "CarpeDiem: Optimizing the Viterbi Algorithm ...", JMLR 10, 2009 [summary]: https://jmlr.org/papers/v10/esposito09a.html
- Vieira, Cotterell & Eisner, "Speed-Accuracy Tradeoffs in Tagging with Variable-Order CRFs and Structured Sparsity", EMNLP 2016 [summary]: https://aclanthology.org/D16-1206/
- "Inside Libpostal", Mapzen blog [summary]: https://mapzen.com/blog/inside-libpostal
- CRFsuite (CQDB feature store) [summary]: https://www.chokkan.org/software/crfsuite
- How spaCy works (strings mapped to integer ids) [summary]: https://explosion.ai/blog/how-spacy-works
- Groot Koerkamp, "PtrHash: Minimal Perfect Hashing at RAM Throughput", SEA 2025 [summary]: https://arxiv.org/abs/2502.15539
- SwissTable overview [summary]: https://pratikpandey.substack.com/p/swisstables-high-performance-hashmaps ; cuckoo hashing: https://en.wikipedia.org/wiki/Cuckoo_hashing
- Cimple (group prefetching, instruction and memory-level parallelism) [summary]: https://arxiv.org/pdf/1807.01624
- Cedar double-array trie benchmarks [summary]: https://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/
- Aho-Corasick (BurntSushi implementation) [summary]: https://github.com/BurntSushi/aho-corasick
- Khuong & Morin, "Array Layouts for Comparison-Based Searching" [summary]: https://arxiv.org/abs/1509.05053
- PostgreSQL thread on an ASCII fast path for normalization [summary]: https://www.postgresql.org/message-id/aFWM9coWT9JUJ8m3%40ubby
- utf8proc vs ICU normalization speed [summary]: https://github.com/JuliaStrings/utf8proc/issues/12 ; https://hackage-content.haskell.org/package/unicode-transforms
- ICU rule-based transliteration performance [summary]: https://hackernoon.com/unicode-transliteration-rules-are-secretly-turing-complete
- re2c [summary]: https://github.com/skvadrik/re2c
- rapidhash / wyhash [summary]: https://github.com/Nicoshev/rapidhash
- PGO study [summary]: https://arxiv.org/pdf/2507.16649
- mimalloc [summary]: https://microsoft.com/en-us/research/blog/mimalloc-a-high-performance-scalable-memory-allocator-for-the-modern-era
- Local: Dataparser (`native/include/aho_corasick.hpp`, `bloom_filter.hpp`, `simd_utils.hpp`, `native/generated/variant_fold.hpp`, `ARCHITECTURE.md`); no single-thread per-address timing was found in its reports.

## 6. Mathematical formulation (for hand-off: another session computes the quantities in 6.8)

Written 2026-09-30. Everything in this section is a definition, a proof sketch or a formula. **No number in 6.4-6.7 has been computed yet**; the
hand-off session fills them in. Where a statement is an unproven assumption it is marked ASSUMPTION.

### 6.0 Rules for the session that computes this

- Do not modify the library or anything under `profiling/golden-v1/` (see `AGENTS.md`). Create a new experiment record `experiments/014-math-quantities/`
  from `experiments/TEMPLATE.md`, save every script, command, stdout/stderr, input hash and output there, and append to `ACTIVITY_LOG.md`.
- Use only the development split `profiling/golden-v1/development.txt` for anything computed from addresses. Model-only quantities (6.6) use the model files.
- Model files: `~/.cache/libpostal-profile/libpostal/address_parser/` (vocabulary trie `address_parser_vocab.trie`; its values are the word counts).
- Baseline figures used below (measured, experiment 013): full set 34.88 µs/address baseline, 13.22 µs build `h`; floor set (non-ASCII) 34.16 µs and 14.67 µs.
- Build `h` source: `experiments/011-normalize-fastpaths/candidate/`. Constants: `DEFAULT_RARE_WORD_THRESHOLD 50`, `UNKNOWN_WORD "UNKNOWN"`,
  `UNKNOWN_NUMERIC "UNKNOWN_NUMERIC"` (`libpostal/src/address_parser.c:18-21`).
- Report every result as "computed" with its inputs; do not turn any of them into a speedup claim. Speed is only claimed from timed experiments.

### 6.1 Notation and the score

An address is tokenized into tokens x_1..x_T (T ≈ 5.9 on average, experiment 001). Labels y ∈ {1..L}, L = 20. For each token t the feature function
emits an ordered list of state features F_t = (f_{t,1}, ..., f_{t,k_t}) and an ordered list of transition ("prev tag") features G_t.
Each state feature f has a sparse weight row w_f ∈ R^L; each transition feature g has a row v_g ∈ R^{L×L}; A ∈ R^{L×L} is the fixed transition matrix.

    s_t(y)      = fl( ... fl( fl(0 + w_{f_{t,1}}(y)) + w_{f_{t,2}}(y) ) ... + w_{f_{t,k_t}}(y) )        (left fold in emission order)
    b_t(y', y)  = the same left fold over G_t using v_g(y', y)
    score(y_1..y_T) = sum over t of [ s_t(y_t) + A(y_{t-1}, y_t) + b_t(y_{t-1}, y_t) ]   (t = 1 has no transition terms)
    output      = argmax over label sequences, computed by Viterbi in the order of crf_context.c

fl(·) is IEEE-754 double rounding to nearest. Rows are sparse: a label absent from a row receives no addition. Evidence for the fold order: build `h`'s
`crf_tagger_score_direct` (`candidate/crf.c`) zeroes the state matrix and adds `data[col]` to `state_scores[indices[col]]` feature by feature in
emission order; Viterbi reads only `state`, `state_trans` and `trans` (comment in `crf_context_set_num_items_lean`).

Decoding is a longest-path problem in the max-plus semiring: alpha_t(y) = s_t(y) + max over y' of [alpha_{t-1}(y') + A(y', y) + b_t(y', y)], cost O(T·L²).

**Feature classes by what they depend on** (from `address_parser_features`, candidate lines 1261-1534):
U (unary: token only: `bias`, `prefix`, `suffix`, n-grams, `sub_word`, `word`), P (pairs: `prev word`, `next word`, `prev word+word`, `word+next word`),
X (position: `first word`), H (phrase: address-dictionary, component and postcode phrase features, which depend on the phrase match, not on one token).
The computational principle ("factorization") is: evaluate each class once at the level it depends on (U per word, possibly at load; P per adjacent pair;
H per phrase), never once per role or per token of a phrase.

### 6.2 Exactness lemmas

**Lemma 1 (floating-point addition is not associative, so regrouping is unsafe).** Example: a = 1, b = 2^-53, c = 2^-53. fl(fl(a+b)+c) = 1 but
fl(a+fl(b+c)) = 1 + 2^-52. Hence precomputing any sub-sum that is not a prefix of the emission order can change s_t and, near ties, the output.

**Lemma 2 (same-order prefix reuse is exact).** For a fixed sequence a_1..a_k, define S_0 = +0.0, S_j = fl(S_{j-1} + a_j). S_j is a deterministic
function of (S_{j-1}, a_j). So if S_m is stored and later S_{m+1}..S_k are computed from it with the same a_{m+1}..a_k, the result is bit-identical to
the full fold. Conditions: (i) no `-ffast-math`, no reassociation, same rounding mode (the operations are additions only, so FMA contraction cannot
occur); (ii) the precomputed fold starts from +0.0 exactly as the `memset` does; (iii) labels absent from every row in the prefix stay +0.0 in the stored
vector, as they do in the original; (iv) the prefix (which features, in which order) must be a function of the key used to look the vector up.

**Corollary (per-word precomputation, report 3.1).** For a token outside every H phrase, the U block (`bias`, then `prefix` if a dictionary prefix
exists, `suffix` likewise, n-grams if word_freq ≤ 50, then `word` if word_freq > 0) is a prefix of F_t and a function of (normalized token,
pre-normalization token, token type). Storing its fold per key is exact by Lemma 2. The same argument applies to the first transition feature
(`word|w` in G_t) when it comes first. Condition (iv) must be proved from the code for each branch (hyphenated words, unknown words, numeric tokens).

**Lemma 3 (prefilter soundness, audit P1/P2).** Let K be a trie's key set and B(K) = { first two bytes of k : k ∈ K, |k| ≥ 2 } ∪ { (b, *) : b is a
one-byte key }. If a token's first two bytes are not in B(K), no key is a prefix of any string starting at that token, so the trie walk from that
token matches nothing. The skip is exact only if the search state machine (`trie_search_tokens_from_index`, states `SEARCH_STATE_*`) ends in the same
state for a skipped token as for a walked token that fails; this is a property of the code to be checked, not of the set argument.

**Lemma 4 (memoization soundness, F1/F2).** If g is a pure function (output depends only on its arguments and immutable model data) then evaluating it
once per distinct argument and reusing the value gives identical results. For F1/F2 the proof obligation is purity: list every global or context field
the function reads (for `word_or_phrase_at_index`, including `long_context`) and show none changes between the calls being merged.

**Lemma 5 (integer keys, report 3.2).** Let V be the vocabulary and D the set of feature-key strings whose name is known and whose value splits by the
separator into parts that are all in V, where no word of V used in D contains the separator. On D, φ(key) = (name id, word ids) is injective, so a table
indexed by φ returns the same row as the string table. Run time uses the same predicate: a feature whose parts are all vocabulary ids goes to the φ table,
anything else to the string table. Exactness reduces to: (a) injectivity of φ on D (check every model key at load), (b) the run-time predicate and
the load-time predicate being the same function.

### 6.3 Certified argmax with exact fallback (research idea, not a plan)

The output is only the argmax. Let S(y) be the true real-valued path score, S_o(y) the original's floating-point score and S_f(y) a faster computation
that sums in a different order. Higham's bound for a recursive sum of n terms: |fl(sum) − sum| ≤ γ_{n−1} · sum |x_i|, with γ_k = k·u / (1 − k·u),
u = 2^-53. Summing the per-token bounds along a path (including the additions Viterbi performs) gives E(y) with |S_o(y) − S(y)| ≤ E(y) and
|S_f(y) − S(y)| ≤ E(y). If ŷ = argmax S_f and S_f(ŷ) − S_f(y) > 2E(ŷ) + 2E(y) for every y ≠ ŷ, then S_o(ŷ) > S_o(y) for all y ≠ ŷ, so the
original also outputs ŷ and ties cannot arise. Otherwise, fall back to the exact computation.
Open problems (why this is research): the check needs a runner-up bound (a 2-best Viterbi costs roughly twice the current 5.2%); E must be computed
cheaply and soundly (a uniform per-address bound using max |w| is simplest); and it only pays off if it enables precomputation that Lemma 2 forbids.

### 6.4 Amdahl: how much must still be removed

With T_base the baseline time and T_now build `h`'s time, reaching speedup S needs saved = T_now − T_base / S. Precomputed from the measured figures:

| Target S | Full set: saved needed (µs of 13.22) | Floor set: saved needed (µs of 14.67) |
|---:|---:|---:|
| 3.5× | 3.25 (24.6%) | 4.91 (33.5%) |
| 4× | 4.50 (34.0%) | 6.13 (41.8%) |
| 5× | 6.24 (47.2%) | 7.84 (53.4%) |
| 7× | 8.24 (62.3%) | 9.79 (66.7%) |

An item with profile share p can at most remove p·T_now. **To compute:** for each item in `AUDIT_REPORT.md` section 3 and report 3.1/3.2, the ceiling
p·T_now from the stage shares in section 2 of the audit, and the sum of ceilings per stage (an upper bound, since shares overlap).

### 6.5 Little's law: a model of memory time

T_mem ≈ N_cold · λ / M, where N_cold = cold cache lines read per address, λ = DRAM latency on this machine (including TLB misses with 16 KB pages),
M = average number of misses in flight. N_cold = sum over lookups of (lines per lookup × probability each line is cold).
Lines per hit today: slot, arena key, `indptr`, `indices`, `data` (up to 5); after C1 about 2; after C1 + integer keys about 1-2. **To compute:**
(1) λ with a dependent pointer chase over a random permutation of a 2.5 GB array; (2) M by running k independent chases (k = 1..16) and finding where
throughput stops rising; (3) the number of distinct lines each address touches, from the lookup ids recorded on the development split (no timing needed),
and a lower bound T_mem_min = N_distinct_cold · λ / M_max. This gives a real floor for the memory part and replaces the UNVERIFIED guesses in earlier answers.

### 6.6 Zipf: coverage of per-word precomputation (model-only, allowed by the generalization rule)

Let c_w be the count stored for word w in the vocabulary trie, sorted descending. Model coverage C(N) = (sum of the top N counts) / (sum of all counts).
Memory M(N) = N · L · 8 bytes dense (20 doubles), or less if stored sparsely. **To compute:** C(N) and M(N) for N = 2^10..2^22; the Zipf exponent s
from a least-squares fit of log c_w against log rank (report the fit range and R²); the largest N with M(N) ≤ 40 MB (leaving margin inside the
~56 MB headroom). Then, on the development split only, the empirical share of plain (non-phrase) tokens whose word is in the top N, to check whether
the model's distribution predicts address text. Words with count ≤ 50 get n-grams and are not candidates.

### 6.7 Reach of integer keys

R = (lookups whose value parts are all vocabulary words) / (all lookups), per feature name. **To compute:** on the development split, dump the feature
keys per address (the original `crf_tagger_score` has a `print_features` path that prints them; call it from a separate harness, do not edit the library),
classify each key by Lemma 5's predicate against the vocabulary, and report R overall and per feature name, plus the number of model keys in D and
whether any vocabulary word used in D contains the separator.

### 6.8 Hand-off checklist (what the next session produces)

| # | Quantity | Formula / method | Inputs | Output |
|---|---|---|---|---|
| Q1 | Amdahl ceilings per item and per stage | 6.4 | audit section 2 shares, 13.22 / 14.67 µs | table |
| Q2 | DRAM latency λ, parallelism M on this M4 Pro | 6.5 (1)(2) | synthetic arrays only | ns, M, raw runs |
| Q3 | Distinct cold lines per address, T_mem_min | 6.5 (3) | development split lookup ids | table + formula result |
| Q4 | C(N), M(N), Zipf s, largest N under 40 MB | 6.6 | vocabulary trie counts | table, fit |
| Q5 | Empirical top-N coverage of plain tokens | 6.6 | development split | table |
| Q6 | Integer-key reach R, size of D, separator check | 6.7 | development split, model keys | table |
| Q7 | Proof obligations for Lemmas 2-5 | list each code branch and whether the condition holds | candidate source | checklist |
| Q8 | (optional) margin distribution vs. Higham bound | 6.3, instrumented harness | development split | histogram, fallback rate |

### 6.9 Errata found by experiment 014 (2026-09-30; the text above is left as written)

1. 3.2 and Lemma 5: the feature separator is `|` (`FEATURE_SEPARATOR_CHAR`), not a space; `feature_array_add` also strips one trailing `|` from each
   non-last piece, and 58 vocabulary words contain `|`, so the run-time predicate must reject pieces containing `|`.
2. 6.5: cache lines on this M4 Pro are 128 bytes (`hw.cachelinesize`), not 64.
3. 6.2 corollary: the U block is not always a prefix of F_t; postcode features precede `bias` for 64% of plain development tokens.
4. Lemma 3: B(K) as defined is insufficient for one-byte tokens (they can begin multi-word keys); use a first-byte set for them.
Results: `experiments/014-math-quantities/README.md`.
