# Actual 021 `l4` profile

Diagnostic only: a `sample` profile of the **unchanged** accepted macOS `l4` executable on the 10,308-address development split. It cannot establish a speedup. The binary was built with Apple clang 17, whole-library PGO/LTO and mimalloc, as recorded in [021 BUILD.md](../../../final/021-l4/BUILD.md). This run made no code or model changes.

## Reproduce and identities

From the workspace root, while no timing/build job is running:

```sh
/bin/bash research/experiments/026-cross-platform/tools/capture.sh profile_run \
  /bin/bash research/experiments/026-cross-platform/profile/run_sample.sh
/bin/bash research/experiments/026-cross-platform/tools/capture.sh profile_report \
  python3 research/experiments/008-profile-after-006/tools/profile_report.py \
  research/experiments/026-cross-platform/profile/parser.sample.txt
/bin/bash research/experiments/026-cross-platform/tools/capture.sh profile_callers \
  python3 research/experiments/008-profile-after-006/tools/callers.py \
  research/experiments/026-cross-platform/profile/parser.sample.txt \
  _platform_memmove,_platform_strlen,_platform_memset,_platform_memchr,fd_find_slot,trie_search_tokens_from_index \
  address_parser_parse,address_parser_features,crf_dict_lookup_array,trie_search_tokens_from_index
/bin/bash research/experiments/026-cross-platform/tools/capture.sh profile_callsites \
  python3 research/experiments/026-cross-platform/profile/analyze_callsites.py \
  research/experiments/026-cross-platform/profile/parser.sample.txt
```

The outer capture files under `../raw/commands/profile_*` give exact arguments, timestamps, stdout, stderr and exit codes. `run_sample.sh` records the AC/thermal gate, before/after environment, binary/input hashes, 500-pass benchmark output and sample output. The profiler began 20 seconds after launch and sampled for 15 seconds at 1 ms intervals. Start 2026-10-01 17:49:19 PDT; end 17:50:16. Benchmark, profiler and capture all exited 0. The executable SHA-256 is `afd0bd84837f859f5e87bd8b54aeda096638e1a8aac3bcc08571844cfa8786c9`; development TSV SHA-256 is `6e97d10f413bb6bbd342e710d30ae3e52ef4b9196875c85180894153dac368b9` ([hashes.txt](hashes.txt)). Model files are the pinned set in [profiling/manifest.json](../../../../profiling/manifest.json); this diagnostic run did not rehash them.

Before the captured run, one invocation of `rtk research/experiments/026-cross-platform/tools/capture.sh ...` failed with exit 127 (`rtk: Permission denied`) because `capture.sh` is not executable. It ran no benchmark or profiler. Its timestamp was not separately captured; the subsequent `/bin/bash capture.sh` invocation above is the recorded successful run.

## Observations

The sample captured **12,695 stack samples**, all on the main parsing thread. **12,543 (98.8%)** were under `address_parser_parse`, so the sample window was inside parsing, not model setup ([raw call graph](parser.sample.txt), [summary](../raw/commands/profile_report.stdout)). The benchmark reports setup 8.939 s and completed 500 passes; the sample ran 20–35 seconds after launch ([parameters](parameters.txt), [benchmark output](bench.stdout)).

Whole-library LTO inlines substantial work into `address_parser_parse`, which appears as 27.8% exclusive without identifying the inlined routine. The broad caller frames separate context fill at `address_parser.c:2159` (**26.6% of parser samples**) from prediction at `address_parser.c:2256` (**67.5%**); these are call-site attributions, not exclusive routine costs ([call-site grouping](../raw/commands/profile_callsites.stdout)).

| Identifiable routine | Exclusive share of all samples | Evidence and code |
|---|---:|---|
| `trie_search_tokens_from_index` | 16.7% (18.4% inclusive) | [summary](../raw/commands/profile_report.stdout); byte transition and restart paths in [trie_search.c](../../../final/021-l4/sources/trie_search.c) lines 416–421 and 487–590 |
| `crf_dict_lookup_array` | 10.9% | [summary](../raw/commands/profile_report.stdout); split, hash and probe in [crf.c](../../../final/021-l4/sources/crf.c) lines 464–501 |
| `address_parser_features` | 6.0% exclusive, 23.9% inclusive | [summary](../raw/commands/profile_report.stdout); nested dictionary and string work overlap |
| libc `memmove`, `strlen`, `memset` | 5.5%, 3.5%, 3.4% | [caller report](../raw/commands/profile_callers.stdout); `memset` is mostly attributed to the inlined prediction call site |
| `feature_array_add_named`, `fd_find_slot` | 4.0%, 3.5% | [summary](../raw/commands/profile_report.stdout); `fd_find_slot` callers are feature generation and integer-key emission |

The AC gate passed. The laptop was busy with unrelated processes (before/after idle CPU 42%/30%; load average 10.8/9.5) in [environment.txt](environment.txt). Shares are diagnostic estimates from one sample window, not cycle counts, timing gains, or proof of cache misses. The 500-pass benchmark output is retained for liveness only and must **not** be compared to unprofiled timing.

## Next code candidate

**First test: a model-derived root transition cache for the phrase and postal-code tries.** At setup, store the exact `(transition index, trie node)` for each byte from `ROOT_NODE_ID` in a small per-trie array. In `trie_search_tokens_from_index`, use it only at `j == 0` when `start_node_id`, `node_id` and `last_node_id` are all `ROOT_NODE_ID` and the original code would enter its transition branch; use the original path otherwise. Leave all continuation, tail, match, and restart logic untouched. The current path computes an index then reads a double-array node for each first byte ([trie.c](../../../final/021-l4/sources/trie.c) lines 107, 384–392; [trie_search.c](../../../final/021-l4/sources/trie_search.c) lines 416–421). This tests whether replacing that dependent node access with a small, exact table helps the largest named hotspot. It differs from the previously rejected first-byte **prefilters**: it performs the same transition, including failed transitions, and replaces the node read rather than adding a skip test. No gain is predicted or measured.

If the root cache fails, examine feature-key construction and hash/probe work next (`crf.c:464–501`). Weight-row co-location is lower confidence from this profile because LTO hides much of scoring inside the 27.8% opaque caller frame; it also needs careful memory accounting. Any new speed candidate needs its own numbered experiment, exact output and single-thread checks, and paired original/candidate timing plus peak/steady memory under [METHOD.md](../../../METHOD.md).
