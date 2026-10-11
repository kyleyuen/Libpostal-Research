# Parallel architecture exploration — final correctness and work evidence

This report summarizes coordinator-owned correctness, operation, fuzz, API and memory screens. Diagnostic/correctness command durations are provenance only; no speed result was measured. The pinned “119” control in the operation tables is the 119-source O2 build from experiment 185, not the packaged PGO/LTO/mimalloc executable.

## Finalist disposition

197 and 199 mimalloc-v2 passed all13 observed-workload output/thread checks, the exact seed-1 fuzz lane (200,000 attempts, 178,955 valid UTF-8 parses), and 78 safe public-API cases. Peak footprint observations come from separate all13 campaigns: O000 vs 197 was 2,611,792,080 B vs 2,611,611,856 B (197 margin 180,224 B); O000 vs 199 mimalloc-v2 was 2,611,677,416 B vs 2,517,404,024 B (199 margin 94,273,392 B). The suite's golden steady-footprint capture reported rounded 2,172 MB for both O000 and 197 after three warm passes, so strict byte-level equality is unavailable; 197's steady gate is inconclusive at that precision. For 199 mimalloc-v2, the same procedure reported 2,156 MB vs 1,927 MB, lower at the suite's precision. The ad-hoc direct-API Python/ctypes run recorded higher peaks for both candidate shared libraries than O000; candidate dylibs were relinked from static archives while O000 used its preserved dylib, so this is an unmatched-profile observation that prevents a blanket lower-memory claim.

The 200k exact fuzz stream records input bytes and ordered output bytes, not only hashes of parser results. Both finalists matched O000 byte-for-byte on all records. It explicitly skips 21,045 invalid UTF-8 inputs because the original's behavior on them is undefined; 178,955 valid inputs were parsed. All thread observations were one. Raw captures: `research/experiments/197-combined-query-removal/raw/fuzz-exact-200k-seed1/` and `research/experiments/199-token-span-storage/raw/fuzz-exact-200k-seed1-mimalloc-v2/`. 197 steady footprint is in `research/suite/results/parallel-20261008-197-memory/`; 199 mimalloc-v2 steady footprint is in `research/suite/results/parallel-20261008-199-mimalloc-v2-memory/`.

| 181-based change | Evidence of work change | Overlap with 119/CPI source already present | Batch disposition |
|---|---|---|---|
| 192 neighbor frequency reuse; 197 composition | 197 skipped 27,661 confirmed repeated state-miss searches; 546,206 guarded previous/next reuse events | Source review shows the neighbor-frequency memo strategy already exists in CPI; the large reuse total is not new CPI savings | 197 passed all13 output/thread and peak-footprint checks; rounded steady memory is inconclusive; only the confirmed-miss skip is novel relative to that strategy |
| 194 length-aware trie lookup | Calls route through a length-aware API; lookup count unchanged | CPI already uses length-aware trie access on a partial batch path; residual dynamic paths still use the old lookup | No claim of complete equivalence or reduced searches |
| 195 immediate context zeroing | 254,204,160 logical zero bytes avoided on golden; physical DRAM writes not measured | CPI's `EXP_NO_RESET`, `EXP_LEAN_CONTEXT`, and `ST_ROW0` flags already avoid this redundant zeroing | No new CPI work reduction established |
| 196 transition view | 12,860 matrix-copy calls skipped; 41,152,000 logical bytes each direction on golden; model-view read count is source-derived one read per cell (25,118,400) | CPI's `EXP_TRANS_ALIAS`/`crf_trans_saved` already avoids this copy | No new CPI work reduction established |
| 197 miss marker + segmented key | 27,661 scorer miss queries skipped and 851,126 temporary key bytes avoided on golden | Miss skipping is novel; segmented word-frequency reuse partly duplicates CPI's existing memo; model occurrence/row multiplicity is unchanged | Strongest 181-composed work candidate, but still 8,954,637 search-API calls across all13 vs 3,350,544 in the pinned 119-source O2 diagnostics; do not claim CPI superiority |

The aggregate allocation instrumentation covers `address_parser.c` and `crf.c`, not `tokens.c`; candidate 199 token-copy/view counters are separate source-site counts and are not allocator-call deltas. “Instrumented search-API calls” use the 185 hook definition, not experiment 187's physical-request event ledger. The 119/O2 control is not the packaged CPI binary. No candidate has a measured speed advantage from this batch.

## Experiment 188 — streamed event scoring

The coordinator created an isolated diagnostic derivative from the tested experiment 185 `prototype181` diagnostic archive, merged the exact experiment 188 `crf.patch`, retained the existing operation/allocation hooks, and replaced only the instrumented scorer loop with the candidate’s streaming loop plus equivalent counter sites. Build commands and outputs are in `raw/188-diag-build-v2/`; derivative source and binary are under `derivatives/188/`. An earlier helper attempt failed before compilation because its output directory already existed (`raw/188-diag-build-attempt1.json`).

Development input is `profiling/golden-v1/development.txt` (10,308 nonempty addresses); full golden input is `profiling/golden-v1/all.txt` (12,863 nonempty addresses). O000, O000 diagnostic, 119-source O2, 119 diagnostic, 188 release and 188 diagnostic outputs all matched byte-for-byte in the completed six-role captures. Every run returned zero and thread samples reported one parsing thread. The six-role golden outputs also matched the frozen `profiling/golden-v1/baseline-outputs.jsonl` comparison. Shared raw output hashes: development `1718ea637659e563bc3965d9995e0acf02e9de28f3c73122d274170cb7d06ad9`; golden `45574ee2b1972f1acf2dbf53eb1fc2c25f8e1972cd5c22e8071fe3606c5c8c63`.

Golden diagnostic counters (12,863 addresses; 75,659 tokens):

| Metric | O000 diagnostic | 119-source O2 diagnostic | Unchanged 181 diagnostic | 188 diagnostic |
|---|---:|---:|---:|---:|
| Instrumented lookup calls | 1,014,421 | 380,614 | 1,058,689 | 1,058,689 |
| Repeated queries | 138,182 | 49,336 | 142,241 | 142,241 |
| Feature keys materialized | 702,370 | 225,266 | 746,635 | 746,635 |
| Feature key bytes | 14,880,342 | 3,228,483 | 15,380,182 | 15,380,182 |
| Direct word IDs consumed | 0 | 0 | 29,608 | 29,608 |
| Direct word fallbacks | 0 | 0 | 3,509 | 3,509 |
| Model rows | 603,376 | 603,376 | 603,376 | 603,376 |
| Weight additions | 14,045,912 | 14,045,912 | 14,045,912 | 14,045,912 |
| Instrumented allocation calls (`address_parser.c` + `crf.c`) | 12,869 | 38,600 | 86,751 | 86,750 |
| Instrumented requested allocation bytes (`address_parser.c` + `crf.c`) | 308,760 | 3,343,294 | 1,166,110 | 1,165,086 |

The exact 181-to-188 diagnostic difference is one fewer instrumented allocation request and 1,024 fewer requested bytes. This is consistent with eliminating the event-array growth request, but does not establish a product memory or RSS improvement. The 185 counters do not directly record feature-event record writes/reads or event-buffer capacity; those fields remain unavailable and are not inferred from unrelated counters. Lookup, model-row and weight totals are unchanged from the 181 diagnostic control. The candidate screen therefore establishes output preservation and a small allocator-request delta; it does not justify a speed claim or a full-13 run by itself. Raw records are `raw/188-development-fullroles-run1/`, `raw/188-golden-fullroles-run1/`, `raw/188-development-119diag-supplement2/`, `raw/188-golden-119diag-supplement1/`, `raw/188-development-181diag-supplement1/`, and `raw/188-golden-181diag-supplement1/`.

Other retained attempts: `raw/188-smoke-run1/` records a coordinator helper return-shape failure after the O000 parser completed; `raw/188-smoke-run2/` is the successful four-role one-address calibration. A redundant six-role run was interrupted after the O000 role when the root narrowed the missing-work request to diagnostic-only supplementary captures; it is retained under `raw/188-development-complete-run2/`. A development supplemental comparison against the full golden frozen output initially failed because development is a subset; the corrected no-frozen comparison passed in `raw/188-development-119diag-supplement2/`. No historical output or source artifacts were overwritten.

## Experiment 191 — segmented trie key

The candidate calls the trie with the exact key as `"word|"` and token segments, avoiding the contiguous temporary string. The isolated diagnostic derivative merges the release delta into the tested 185 `prototype181` hooks and records the segmented request using a diagnostic-only concatenation identity adapter. A two-query calibration (one whole key and one two-segment key, same dictionary/operation) returned 2 lookups and 1 repeat, matching the original key identity. The adapter is not part of the release candidate and its temporary join is excluded from product-key/allocation metrics.

The development and golden six-role runs all exited 0, matched O000 byte-for-byte, and sampled one parsing thread per role. Golden candidate release and diagnostic outputs match the frozen 12,863-row reference with no missing addresses or ordered-component differences. Raw evidence: `raw/191-development-run1/`, `raw/191-golden-run2/`, `raw/191-golden-run2/frozen-comparison.json`, and canonical-identity candidate-only supplements under `raw/191-*-canonical-metric-supplement1/`.

| Diagnostic metric | 181 base, dev | 191, dev | 181 base, golden | 191, golden |
|---|---:|---:|---:|---:|
| Instrumented lookup calls | 848,149 | 848,149 | 1,058,689 | 1,058,689 |
| Repeated queries (canonical identity) | 113,744 | 113,744 | 142,241 | 142,241 |
| Temporary `word|TOKEN` key constructions | 59,186 | 0 | 73,873 | 0 |
| Temporary key text bytes, excluding NUL | 622,974 | 0 | 777,253 | 0 |
| Instrumented allocation calls | 69,508 | 10,322 | 86,751 | 12,878 |
| Requested allocation bytes | 934,944 | 252,784 | 1,166,110 | 314,984 |
| Model rows / weight additions | 483,550 / 11,261,655 | same | 603,376 / 14,045,912 | same |

The allocation call and byte deltas match removing the short temporary key allocations (the allocated size includes the NUL byte). Instrumented search-API calls and repeated searches are unchanged. Trie-edge and compressed-tail comparison counts are unavailable in parser metrics; a separate synthetic helper validated six hits and five misses, including UTF-8, empty segment boundaries, and a long compressed tail, but those helper operations are not parser counts. Six additional all13 workload captures were stopped after the repeat-identity encoding issue was found; their completed output records remain valid, but their diagnostic repeat totals are retired. See `raw/191-all13-run1/interrupted.json`. Corrected candidate-only dev/golden diagnostic captures pass; the later 197 composition uses the canonical identity adapter in its all13 metrics.

## Experiment 192 — reuse neighbor word frequency

The candidate reuses the same-address front-record frequency for previous/next plain-word context only when the saved token index is valid and the exact normalized-word pointer matches; otherwise it calls the original vocabulary resolver. Development and golden six-role captures passed output equality and single-thread checks. The golden candidate also matches all frozen reference rows and ordered components.

| Metric | 181 baseline, dev | 192, dev | 181 baseline, golden | 192, golden |
|---|---:|---:|---:|---:|
| Instrumented lookup calls | 848,149 | 800,803 | 1,058,689 | 999,582 |
| Previous / next exact-guard frequency reuse | — | 26,571 / 20,775 | — | 33,179 / 25,928 |
| Guard fallback queries | — | 0 | — | 0 |
| Repeated queries | 113,744 | 66,398 | 142,241 | 83,134 |
| Model rows / weight additions | 483,550 / 11,261,655 | same | 603,376 / 14,045,912 | same |
| Instrumented allocation calls / bytes | 69,508 / 934,944 | same | 86,751 / 1,166,110 | same |

The measured search reduction equals the previous-plus-next guarded reuses. This is a structural query-count result, not a speed claim. Despite removing 59,107 golden lookups versus the 181 baseline, 192 still executes 999,582 instrumented lookups, substantially above the pinned 119-source O2 control's 380,614 golden lookups. The 119 control is not the packaged PGO/LTO/mimalloc executable, so this comparison does not establish that 192 beats packaged CPI. See `raw/192-development-run1/`, `raw/192-golden-run1/`, and `raw/192-golden-run1/frozen-comparison.json`.

## Experiment 193 — skip confirmed word-feature misses

This candidate carries a completed miss for an exact `word|TOKEN` state-feature lookup from the fused front pass into the scorer. The normal feature emission remains present, so model-row multiplicity and output semantics are unchanged; only the duplicate scorer trie query is skipped when the marker still names that exact emitted feature slot.

The six-role development and golden lanes passed: preserved O000 release, O000 diagnostic, 119-source O2 release and diagnostic controls, and 193 release and diagnostic all exited 0, matched O000 byte-for-byte, and each role reported one parsing thread. Golden frozen-v1 comparison passed for all six outputs (12,863 rows). The separately preserved experiment-119 packaged CPI executable also passed development and golden output equality and one-thread checks; its golden output matched the frozen reference. This packaged CPI check is correctness-only and has no diagnostic counters. The 185-built 119-source O2 control remains a separate comparator and is not the packaged binary. Raw records: `raw/193-development-run2/`, `raw/193-golden-run1/`, `raw/193-golden-run1/frozen-comparison.json`, `raw/193-development-packaged-cpi-release-supplement1/`, and `raw/193-golden-packaged-cpi-release-supplement1/`. The first development attempt mistakenly used the profiling `parser_profile` CLI instead of the baseline dumper; exit 2 and usage output are retained in `raw/193-development-run1/`. Batch negative controls are in `raw/negative-output-control/`: byte equality rejected an altered first byte, and the frozen comparator rejected one altered component with zero missing rows.

The immutable 193 release library SHA-256 is `594b84ac7146ce20f4a67922cf680dc193ad57747cca60846236f0ff49179b0e`; its handoff source-manifest SHA-256 is `93783275cc5692fa4e659998126dcc8b26c48ddac902feb2ac15a767af45145b` (see `research/experiments/193-skip-confirmed-word-miss/raw/handoff.json` for the full digest). The private diagnostic derivative is built from the tested 185-hook 181 source with the exact 193 source delta; it adds counters for completed miss prefill, marker attachment, and skipped duplicate scorer query. Build commands, exact source hashes, archive/dumper hashes, stdout/stderr, timestamps and exits are in `raw/193-diag-build-v1/`; the derivative is under `derivatives/193/`. Candidate-specific source hashes: `crf.c` `4bc0af55b44e43d46b490b06af20d7d373f76fbdc350c2057c7c69795b3704d8`, `address_parser.c` `11c47b19a96847135564042283a51bbc7ca33b822ee426b1a92256c73b83a2a2`, and `diag.c` `144a280f87702690928dd2be549c3fa44b891f23bbc2082d4806ea2e1a1eed2b`.

| Diagnostic metric | 181 base, dev | 193, dev | 181 base, golden | 193, golden |
|---|---:|---:|---:|---:|
| Instrumented lookup calls | 848,149 | 845,332 | 1,058,689 | 1,055,180 |
| Repeated queries | 113,744 | 110,927 | 142,241 | 138,732 |
| Completed front-word misses | — | 17,900 | — | 22,299 |
| Markers attached / scorer searches skipped | — | 2,817 / 2,817 | — | 3,509 / 3,509 |
| Model rows / weight additions | 483,550 / 11,261,655 | same | 603,376 / 14,045,912 | same |
| Instrumented allocation calls / requested bytes | 69,508 / 934,944 | same | 86,751 / 1,166,110 | same |
| Direct-word fallbacks | — | 0 | — | 0 |

The lookup delta exactly matches marked misses consumed at their corresponding scorer slots. This is a small structural work reduction (2,817 development and 3,509 golden searches, about 0.33% of the 181 counts), with no allocation or model-work change. Counters do not imply elapsed-time or RSS improvement. Experiment 193 itself was not advanced to all13; its miss-skip change is included in the all13-validated 197 composition.

## Experiment 198 — certified float32 Viterbi v3

This candidate keeps the existing exact f64 score matrices and attempts a float32 max-plus recurrence only when the runtime guard and propagated error bound certify the selected path; otherwise it calls the original exact decoder. The six-role development and golden lanes passed byte equality and one-thread checks. All six golden role outputs matched frozen-v1 (12,863 rows). The experiment-119 packaged CPI output was checked in 193 on the same immutable inputs; this batch reuses that correctness evidence, while the 185-built 119-source O2 controls ran in both 198 lanes.

A private derivative from the 198 v3 source uses the 185 diagnostic library/archive and adds decoder-specific counters. Build failures are retained in `raw/198-diag-build-v1/`, `raw/198-diag-build-v2/`, and `raw/198-diag-build-v3/`; build v4 is `raw/198-diag-build-v4/`. A candidate-only supplementary run rebuilt the derivative with an additional bound-matrix-cell counter (v5 build logs `raw/198-diag-build-v5/`, source under `derivatives/198/diagnostic-v5/`). That candidate-only run passed output/thread/frozen checks on both inputs and preserved the byte-identical control captures. Its diagnostic archive SHA-256 is `b2be4584ea6ad852f1be1a6228071f3753f8810da13abda2e00e976576a89177`; dumper SHA-256 is `650f08bf826a173bfa230d609aa3b83e918d803f53ef59a767a1bf27ce0213c9`.

| Decoder metric | Development | Golden |
|---|---:|---:|
| Certificate attempts | 10,305 | 12,860 |
| Certified accepts | 10,305 | 12,860 |
| Exact fallbacks | 0 | 0 |
| Float recurrence predecessor cells | 20,115,200 | 25,118,400 |
| Bound-pass transition-matrix cells | 20,115,200 | 25,118,400 |
| Certificate predecessor/final-label comparisons | 1,211,860 | 1,513,120 |
| Exact fallback predecessor cells | 0 | 0 |

The host accepted every observed attempt; no unsupported-architecture, FP-mode, numeric-range, or inconclusive-bound fallback occurred. The v5 counter establishes a separate `(T−1)×L×L` transition-matrix scan for the propagated error bound; it is as large as the float recurrence predecessor-cell count (25,118,400 golden cells), so the certificate is not an O(TL)-only check. Source-derived accounting adds the bound scan's transition-score reads and per-cell magnitude/error arithmetic; these are logical operations, not separately hardware-measured memory traffic. Decoder counters show additional work and path acceptance, not elapsed-time savings. The diagnostic candidate `crf.c` was rebuilt from the 119 source without the 185 model-row/weight hooks, so model-scoring counts and candidate-side full-pipeline allocation/lookup counters are unavailable in this derivative; its zero-valued uninstrumented cells must not be interpreted as measured zero. No speed or memory promotion claim follows from these diagnostic runs. Raw captures are `raw/198-development-run1/`, `raw/198-golden-run1/`, `raw/198-development-bound-counters-supplement1/`, `raw/198-golden-bound-counters-supplement1/`, and `raw/198-golden-run1/frozen-comparison.json`.

## Experiments 194–197 — length/reset/view and combined query changes

Release-only candidate captures for 194–197 and 199–200 reused the retained O000 stdout for the same input files. Each development output matched its 10,308-row O000 byte stream; each golden output matched its 12,863-row stream and frozen-v1 reference. Every captured thread sample was one. The exact command sequence is `raw/release-parity-194-200-sequential.log`; individual records are `raw/{ID}-{development|golden}-release-parity-supplement1/`. Matching O000, original diagnostic, 119-source O2 release, and 119-source O2 diagnostic evidence is retained from the earlier six-role lanes; “119-source O2” is not the packaged PGO/LTO CPI executable.

Candidate-only diagnostics for 194–196 and 197 also matched O000 byte output, golden frozen-v1, and one-thread checks. Diagnostic builds retain every failed attempt; successful artifacts and source hashes are in `derivatives/{194,195,196,197}/artifacts/diagnostic-v{4,5}/` and corresponding `raw/*-diag-build-*` manifests.

| Candidate diagnostic metric | 194 dev / golden | 195 dev / golden | 196 dev / golden | 197 dev / golden |
|---|---:|---:|---:|---:|
| Instrumented search-API calls | 848,149 / 1,058,689 | 848,149 / 1,058,689 | 848,149 / 1,058,689 | 797,986 / 996,073 |
| Repeated-query calls | 113,742 / 142,239 | 113,744 / 142,241 | 113,744 / 142,241 | 63,581 / 79,625* |
| Length-aware lookups / key-scan proxy bytes avoided | 706,341 / 12,892,934; 881,645 / 16,099,269 | — | — | — |
| Duplicate context-zero logical bytes / calls avoided | — | 203,592,480 / 20,610; 254,204,160 / 25,720 | — | — |
| Transition copy calls avoided / logical bytes read and written | — | — | 10,305 / 32,976,000 each direction; 12,860 / 41,152,000 each direction | — |
| Segmented-key calls / joined temporary bytes avoided | — | — | — | 59,186 / 682,160; 73,873 / 851,126 |
| Previous / next guarded frequency reuses | — | — | — | 26,571 / 20,775; 33,179 / 25,928 |
| Confirmed-miss scorer lookups skipped | — | — | — | 2,817 / 3,509 |
| Model rows / weight additions | unchanged 483,550 / 11,261,655; golden 603,376 / 14,045,912 | same | same | same |
| Aggregate instrumented allocation calls / requested bytes (`address_parser.c` + `crf.c`) | 69,508 / 934,944; golden 86,751 / 1,166,110 | same | same | 10,322 / 252,784; golden 12,878 / 314,984 |

Interpretation is limited to the named source counters. Candidate 194 does not reduce the number of instrumented search-API calls; it routes 706,341 development and 881,645 golden calls through a length-aware API. Its `repeated_queries` delta of −2 is an unresolved identity-tracking effect, not a search saving. The byte count is a proxy for avoided per-query `strlen` traversal, not CPU time. Candidate 195 removes logical score-matrix zeroing that is immediately repeated by reset; its byte total represents logical element volume, not measured DRAM traffic. Candidate 196 avoids one `L×L` model-transition copy per combined Viterbi inference; read/write totals are logical payload sizes, not memory-bandwidth measurements. Candidate 197 combines segmented key construction, neighbor frequency reuse, and miss-marker skipping. On golden input its search-API delta is 62,616 (59,107 guarded reuses plus 3,509 skipped confirmed misses); model-row/weight multiplicity is unchanged. Its feature-key aggregate drops by 73,873 records / 777,253 bytes because front-prefill keys are no longer materialized contiguously; that aggregate is not a count of suppressed model features. Segmented-prefill and joined-key byte counters are separate from scoring feature occurrences. The corrected v7b diagnostic uses canonical joined-key identity and all13 metrics are valid; the earlier v5 repeat identity is retired. See `raw/diag-parity-194-196-candidate-tus-v2.jsonl`, `raw/diag-parity-196-199-200-sequential-v3.jsonl`, `raw/197-diag-build-v7b/`, and `raw/197-all13-diagnostic-v7b/`.

## Experiments 199–200 — token spans and fused ASCII pass

199 and 200 release and diagnostic captures passed development/golden byte equality and one-thread checks. 199's restored v9 diagnostic derivative includes the 185 lookup/stage hooks plus token-specific counters; all13 captures passed and are at `raw/199-all13-diagnostic-v9/` and `raw/mimalloc-all13-v2/`. The original O2/system-allocator artifact had an all13 peak 210,403,520 B above O000; the allocator-only mimalloc-v2 relink passed the matched all13 peak comparison as detailed in its experiment README. Its measured golden token counters: 12,139 eligible / 724 fallback addresses, 68,373 view records (400,388 payload bytes), 7,286 copied fallback calls (27,993 payload bytes). These are source-site counts, not allocator-call or RSS reductions; generic allocation hooks do not instrument `tokens.c`.

199 mimalloc-v2 also passed exact seed-1 fuzz on 200,000 attempted cases and the 78-row safe API output contract. Fuzz bytes include every generated input and ordered output tuple, not only an FNV digest; the 21,045 invalid UTF-8 cases are explicitly skipped. API output is byte-identical, but the unmatched shared-library Python/ctypes capture had a higher peak than O000. 199 does not have a measured speed result; the accepted PGO profile is not built, and this mimalloc-only build retains scanner O0.

200's restored-hook diagnostic captured 24 accepted of 12,863 fused-ASCII attempts on golden; 12,839 addresses fell back after scanning 209,630 bytes. The fallback path still made 12,839 original normalization and tokenization calls and copied 501,486 normalized-buffer bytes. This screen does not establish a broad pipeline-work reduction; 200 is deferred. Four invalid-api edge records remain out of the shared 78-case safe selector.

## Remaining batch

The candidate queue is complete for this finite batch. Experiments 188–196 remain preserved as development/golden research screens; later combined candidate 197 received all13 validation. Experiments 198 and 200 passed development/golden correctness but add work or offer little observed coverage, so they are deferred. Candidate 199 mimalloc-v2 passed all13, exact fuzz, safe API output and the accepted peak/steady corpus memory checks, but speed remains unmeasured and API-harness peak was higher under the unmatched dylib build. Candidate 197 passed all13, exact fuzz, safe API output and the corpus peak check; steady memory rounded to the same MB as O000 and is inconclusive at strict precision. No performance promotion is claimed. All attempted build/run versions, failed checks and source snapshots remain retained. See `queue.json` for final states.
