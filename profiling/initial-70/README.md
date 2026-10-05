# Libpostal single-threaded parser profile

Measured September 28, 2026 on Apple M4 Pro, 24 GiB RAM, macOS 26.5.2.
Checkout: `25099c506612b34b23b1bfe286ca6321fcf06f35` (libpostal 1.1.4).
Compiler: Apple clang 17.0.0. Default upstream library optimization: `-O2`,
scanner `-O0`; native arm64, SSE disabled. Harness statically linked, `-O2 -g`.
No tracked libpostal source files were modified.

## Workload and method

70 unique literal inputs extracted from upstream `test/test_parser.c` calls
to `test_parse_result_equals`. Embedded line breaks were replaced by spaces
so each address occupies one line in `addresses.txt`. This includes multiple
countries/scripts and short/incomplete inputs; it is a small regression corpus,
not a representative production or held-out benchmark. Repeated corpus traversal
favors hot caches. Results must not be generalized to millions of unique addresses.

Default base and parser model archives from upstream release v1.0.0, as selected
by this checkout's version files. Models are stored in
`/Users/kyle/.cache/libpostal-profile/libpostal` (about 1.8 GiB on disk).
The language classifier is not loaded because this benchmark calls the parser.

One process and one parsing thread. Setup and a full warm-up traversal occur
before timing. Each measured operation calls `libpostal_parse_address` and
destroys its response. Every response is checked for non-null/nonempty output;
this is not a full expected-output correctness test. No input/output printing
inside the timed loop. No CPU-core pinning or hardware performance counters.

Three independent 10-second timing runs without a profiler:

| Run | Addresses/s | Microseconds/address | Peak RSS bytes |
|---|---:|---:|---:|
| 1 | 45,436.38 | 22.01 | 2,611,396,608 |
| 2 | 46,142.62 | 21.67 | 2,611,232,768 |
| 3 | 45,852.89 | 21.81 | 2,611,036,160 |

Median: **45,852.89 addresses/s**. Peak RSS: **2.432 GiB**. RSS comes from
macOS `getrusage`, includes startup/model loading, and is not model-file size.
Measured CPU time was 9.984–9.990 seconds per roughly 10 seconds wall time.

Separate 30-second run sampled for 15 seconds at a requested 1 ms interval using
macOS `sample`, after READY/warm-up. Collected 12,860 main-thread stack samples.
Percentages below are statistical stack-sample shares, not exact instrumented
CPU-cycle counts. Optimizer inlining can affect attribution.

## Observed hotspots

Inclusive shares include callees and **overlap; do not sum these rows**.

| Function/stage | Inclusive sample share |
|---|---:|
| `crf_tagger_score` (includes feature generation and lookups) | 65.2% |
| `trie_get_data` (all calling sites) | 28.0% |
| `address_parser_features` | 19.1% |
| `normalize_string_latin` (all calling sites) | 13.2% |
| `char_array_add_vjoined` (feature string joining) | 12.7% |
| `address_parser_context_fill` | 9.0% |
| `crf_context_viterbi` | 4.0% |
| `crf_context_reset` | 3.2% |
| `crf_context_set_num_items` | 3.0% |
| `scan_token` | 0.9% |

Largest exclusive shares: trie traversal (`trie_get_from_index`) 21.1%,
`crf_tagger_score` itself 12.7%, feature-string joining 10.7%, memset 4.2%,
Viterbi 4.0%. Full inclusive/exclusive counts are in `profile-summary.json`.

## Implications for optimization

1. Investigate feature construction plus lookup first: avoid constructing and
   rescanning full temporary feature strings where equivalent incremental trie
   traversal or known lengths suffice. Preserve separator normalization and exact
   feature semantics. A replacement hash table or result cache must be evaluated
   against the no-extra-memory constraint, not assumed to satisfy it.
2. Inspect CRF sparse weight accumulation/locality: scoring itself has a substantial
   exclusive share. Hardware counters would be needed to distinguish instruction
   cost from memory stalls.
3. Investigate inference-only context work. `crf_read` enables marginal buffers,
   `crf_context_set_num_items` clears state/state-transition matrices, and the
   following reset clears them again. Removing provably unnecessary work could
   help speed and memory, but requires checking all supported inference APIs.
4. Normalization remains meaningful. Any ASCII shortcut needs identical behavior
   and a Unicode fallback; this profile does not establish that such a shortcut
   will produce a particular gain.

Viterbi and scanner tuning alone cannot deliver a large end-to-end gain on this
workload: even eliminating either entirely would yield only about 1.04x and
1.01x respectively. Eliminating the whole CRF scoring stage (65.2%) would have an
approximate ceiling of 2.87x if everything else stayed fixed. Therefore **3.5x
requires substantial gains across scoring and other stages**, or a different
bottleneck distribution on the actual target corpus. This profile establishes
credible optimization targets, not proof that 3.5x is achievable.

## Reproduce

From the workspace root (quote the root path when changing directory):

```sh
clang -O2 -g -Ilibpostal/src profiling/parser_profile.c libpostal/src/.libs/libpostal.a -o profiling/parser_profile
profiling/parser_profile profiling/addresses.txt 10
```

For stack sampling, run the harness with a 30-second duration, wait for the
`READY pid=...` line, then in another terminal:

```sh
sample PID 15 1 -file profiling/parser.sample.txt
python3 profiling/summarize_sample.py
```

Replace PID with the printed process ID. Do not use profiler-attached timings
as the baseline. `summarize_sample.py` checks call-tree count consistency.

Build setup used Homebrew autoconf, automake and libtool, then `./bootstrap.sh`,
`./configure --disable-data-download --disable-sse2 --datadir=/Users/kyle/.cache/libpostal-profile`,
and `make -C src -j4 libpostal.la`. Build/download logs are in the workspace root
as `profile-*.log`. Generated build files remain in the checkout for reproduction.
