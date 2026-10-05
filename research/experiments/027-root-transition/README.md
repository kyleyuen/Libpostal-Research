# Experiment 027 — exact root-transition cache for parser phrase tries

Created 2026-10-01T17:57:28-07:00 from TEMPLATE.md, before source edits. **Decision: rejected** after a development-only screen. The cache passed exactness checks but did not produce a material speed gain, and measured peak memory footprint increased relative to unchanged 021. There is no final speedup claim.

## Hypothesis and source

The actual unchanged 021 l4 profile in [026/profile](../026-cross-platform/profile/README.md) attributed 16.7% of parser samples exclusively (18.4% inclusively) to trie_search_tokens_from_index. Reusing an exact model-derived root transition (index, node) at the first byte of a fresh token might reduce dependent reads. This is a structural hypothesis independent of benchmark inputs. Earlier phrase prefilters failed and were not repeated.

The candidate adds two 256-entry caches to address_parser_t in [sources](sources/), populated after the phrase and postal-code tries load. The parser uses the cached pair only when the search and previous node are root at byte zero; other transitions and generic trie callers keep the original path. The cache source pointer must match the trie. These parser-owned caches are rebuilt on parser reload and rely on the two loaded parser tries remaining immutable during parsing. The public generic trie_t remains mutable, so the cache is not enabled there. No thread, worker, or cross-address result cache was added. A unique [cache header](sources/root_transition_cache.h) avoids mixing the candidate and upstream trie_search.h include guards.

Base commit: libpostal checkout 25099c506612b34b23b1bfe286ca6321fcf06f35, confirmed in [source_commit](raw/commands/source_commit.stdout). The source base is preserved research/final/021-l4/sources. The unchanged-source control artifact exactly matches that directory, and the candidate artifact exactly matches this experiment's sources ([control identity](raw/commands/control_source_identity.meta), [candidate identity](raw/commands/cache_source_identity.meta)). No 021 or checkout source was edited. The production [source patch](source.patch), including both added headers, has SHA-256 01095c0c325b185d282c90479baa3908eb3a2f042db720d3ac8e566f754b6495; [final hashes](raw/commands/final_hashes_r2.stdout) list individual files. The old unbuilt global-cache draft was removed before the first build.

## Reproduction and environment

Machine: Apple M4 Pro, macOS 26.5.2, Apple clang 17 ([compiler](raw/commands/compiler_version.stdout), [OS](raw/commands/os_version.stdout)). [build_cc.sh](tools/build_cc.sh) copies sources into separate artifact paths, uses the same 021 switch set, Apple clang -O2 -flto for the library, the preserved -O1 scanner object, -flto link, mimalloc object, and fresh development-only LLVM PGO per variant. Complete invocation, settings, timestamps, stdout/stderr and exit codes are in [build_control_r2](raw/commands/build_control_r2.meta), [build_cache_r2](raw/commands/build_cache_r2.meta), [control settings](artifacts/control_r2/build_settings.txt) and [checker link repair](raw/commands/link_cache_r2_checker.meta). cache_r2 completed production library and harness links; its script returned 1 while compiling the auxiliary checker because its command lacked LIBPOSTAL_DATA_DIR. The corrected script and captured standalone link produced the checker. Candidate libpostal.a SHA is 526abf1aba0a23941827d66fd1f150198c3f1336cbc726b16141c079d11b70e7; control is 67990b9dd4489f2222664a570716e12c0d2b5548b3def2e2f62856c417f1314e. Candidate bench_ext SHA is c8819a23336579b43c8ce961465d448b6bd557fd2f29126845c0e22884fed517; control is 9da82355d74f28ebb08d231285199905b266806e92ffdaf3478c810215a47f51.

The development TSV SHA-256 is 6e97d10f413bb6bbd342e710d30ae3e52ef4b9196875c85180894153dac368b9. Golden-v1 all.txt is 02252919fc2a24a3f0af41f3f7c5d6f8aaed6a5eb9548a2dcff6a4fcfe5f6c91; novel novel.txt is a424c53e75d3d55f4172f7b13a59bf5ece961bf0f4acd7b85e214ce0aee09d47. The four active model hashes were rechecked against profiling/manifest.json: CRF e44b308e9e1942efe8ad8226929e29ca49e9ea2900ed36625c601c53509ba5c1, phrases 739643ea06d8c492ba57bd51ba1dc1243853e7f8c6130e3a7bd30150221119e6, postal codes 7c5b20c541c16758cc73c41025f2327be525831dff272a6b4c3aeb901e481b1b, vocabulary 962a985f29530440035bd379826f38e35f0f0fc718926444658d89bbd70829b1. Paths and all other hashes are in [final_hashes_r2](raw/commands/final_hashes_r2.stdout); scanner/mimalloc hashes are [here](raw/commands/scanner_mimalloc_hashes.stdout).

## Correctness and single-thread gate

[verify.sh](tools/verify.sh) and its [completed run](raw/commands/verify_r2.meta) compare the control and candidate against the frozen original-library reference, with every output ordered and byte-identical:

| Check | Result | Raw evidence |
|---|---|---|
| Model-derived root entries | 512/512 exact; deliberate table corruption detected | [ordinary versus cached](raw/commands/cache_root_check.stdout), [negative](raw/commands/cache_root_negative.stdout) |
| Phrase-search behavior | 20,616 ordinary-versus-cached searches on 10,308 development addresses; 0 mismatches | [checker](raw/commands/cache_root_check.stdout) |
| Golden-v1 | Control and candidate each match frozen original for all 12,863 outputs; candidate equals control byte-for-byte | [candidate frozen check](raw/commands/cache_r2_frozen_v1.stdout), [control frozen check](raw/commands/control_r2_frozen_v1.stdout), [byte comparison](raw/commands/cache_cmp_control_v1.meta) |
| Novel | Same for 12,863 outputs | [candidate frozen check](raw/commands/cache_r2_frozen_novel.stdout), [byte comparison](raw/commands/cache_cmp_control_novel.meta) |
| OSM | Same for 11,405 outputs | [candidate archived comparison](raw/commands/cache_r2_cmp_osm.meta), [control comparison](raw/commands/control_r2_cmp_osm.meta) |
| Seeded fuzz | 2,000,000 cases byte-identical to archived original result | [comparison](raw/commands/cache_fuzz_cmp_s1.meta) |
| Output-check negative control | One component changed in a disposable copy; frozen checker reports exactly one mismatch and exits 1 | [negative output](raw/commands/frozen_negative_v1.stdout), [creation](raw/commands/make_negative_output.meta) |

An untimed candidate run showed exactly one thread in [ps -M](raw/commands/threads_cache_r2.stdout); the source change adds no thread primitives. The negative output copy and corrupted in-memory cache never altered the frozen reference or production candidate output.

## Development diagnostic and memory

The source comparison is control_r2 versus cache_r2, each built from fresh development PGO. [run_dev_diagnostic.sh](tools/run_dev_diagnostic.sh) alternated order over five paired processes, with 120 timed passes of the **same 10,308-address development TSV** per process and the same novel TSV as a separate untimed warm-up. Only passes 2–120 are pooled below; first-pass and setup measurements remain in each raw log. The diagnostic ran 2026-10-01 18:20:07–18:24:32 PDT ([session](raw/commands/diagnostic_session_r1.meta), [analysis](raw/commands/diagnostic_analysis.stdout)). Every run passed the AC/Low Power/thermal gate. The host was busy throughout: session load averages were about 7.2–8.0, system CPU was about 36%, and only 39/595 control and 50/595 candidate warm passes met the protocol's clean-pass rule. The top-pipe sampler files are empty, an instrumentation failure; per-run [environment snapshots](raw/dev_diagnostic_r1/), CPU-time and context-switch fields in bench_ext, and /usr/bin/time -l remain. These are **noisy screening data**, not final validation.

| Pair | Control median addr/s | Cache median addr/s | Cache/control |
|---:|---:|---:|---:|
| 1 | 125,456.9 | 125,937.5 | 1.00383 |
| 2 | 125,148.7 | 124,687.5 | 0.99631 |
| 3 | 126,325.4 | 124,590.8 | 0.98627 |
| 4 | 125,534.5 | 126,110.6 | 1.00459 |
| 5 | 125,611.0 | 125,546.8 | 0.99949 |

Paired ratio median **0.99949**, range **0.98627–1.00459**. Pooled warm medians were **125,636.8** control and **125,380.7** candidate addresses/s (candidate/control **0.9980**); P10 rates were 120,067.3 versus 119,903.1. Median CPU use was **7.957** versus **7.973 µs/address**. The variation exceeds the putative effect. No original-baseline or validation-corpus timing was run because this development screen did not justify promotion.

Relative to the unchanged 021 control, /usr/bin/time -l recorded the same peak footprint in each of five trials per variant, **2,470,578,480 bytes control** versus **2,476,722,480 bytes candidate**, a **+6,144,000-byte** incremental difference. A separate untimed [late footprint sample](raw/commands/footprint_control_r2.stdout) was 2250 MB control versus [2254 MB candidate](raw/commands/footprint_cache_r2.stdout); that single sample is approximate. The hard no-increase gate compares with the preserved original, which was not rerun in this 027 timing session, so original-relative memory acceptance is **not reverified here**. The candidate footprint appears below the earlier approximately 2.613 GB original result, but mixing sessions cannot establish that gate. Peak RSS varied with swap and is not the memory criterion. The exact cause of the incremental footprint was not isolated because the candidate already lacked a speed gain.

## Failed attempts, limitations and decision

All attempts remain in raw/commands/:

- build_control failed after PGO training because the first builder passed a profdata path containing spaces as two compiler arguments. build_control_r2 passed after quoting correction.
- build_cache failed compilation because an upstream trie_search.h was included before the candidate header with the same guard. The API moved to root_transition_cache.h; build_cache_r2 then built the production artifacts but failed only on the auxiliary checker link's missing data-directory macro. link_cache_r2_checker repaired that link, and the builder was corrected for future reproduction.
- verify_r1 failed immediately because capture.sh lacked execute permission; chmod_tools corrected it, and verify_r2 passed.
- final_hashes failed on a nonexistent address_parser.dat path. The active model is address_parser_crf.dat; final_hashes_r2 passed and matches the pinned manifest.

Decision: **reject 027 as a performance candidate**. Exact output and single-thread requirements passed, but this source change showed no material development gain and increased observed peak footprint relative to unchanged 021. Its original-relative memory gate remains unverified in 027. No validation or cross-platform performance claim follows from it. The result does not alter the verified 021 result. Parent integration should append this rejection to the experiment ledger and activity log; the professor report needs no new speed figure.
