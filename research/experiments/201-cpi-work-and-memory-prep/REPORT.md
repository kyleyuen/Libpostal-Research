# Experiment 201 — CPI token-span work and memory screen

Date: 2026-10-09 (America/Los_Angeles)  
Status: bounded evidence complete; no promotion claim  
Protocol: `research/METHOD.md`

## Question and decision

Does 199’s token-span storage remove real token-payload work relative to CPI, while preserving outputs, single-thread parsing, and the original memory gate? The paired diagnostic evidence shows that it removes most retained-token payload copying on the observed corpus. The focused uninstrumented suite screen is byte-correct and single-threaded, with lower peak footprint than 000 and equal rounded steady footprint. Three timing rounds show 199 close to source-matched CPI and slower than accepted packaged CPI by the warm median. This is a bounded screen, not a promotion: steady-footprint precision is rounded, the sample is one golden corpus, and the 78-case native API peak is marginally above matched CPI.

## Frozen identities and preservation

Candidate source is the immutable 199 token-span snapshot (source manifest SHA-256 `eb83c36944062bb8db12add9ded4789b12d7f2b3f993463c3654cc6822b3c0fb`). The allocator-only v2 relink uses the same 199 archive SHA-256 `447e00b40cff2dbb2c4b388b8a844fd01bfc1610463f93432de3a2eb4960f576`, mimalloc object `03460b36edb0f104270109d33d96f8e7fc03c08d326cc5c2182c34b0a4900b82`, no PGO, O2/LTO and scanner O0. The source-matched CPI comparison uses the same no-PGO O2/LTO/scanner-O0/mimalloc profile; packaged CPI has its accepted PGO/scanner-O1 profile and is reported separately. The original executable/library remain the pinned 000 baseline.

Diagnostic capture does not time product performance. The paired diagnostic build differs only by `EXP_TOKEN_SLICES`, with the same CPI-derived diagnostic hooks, flags, model, input order, and matched archive. Allocator counters cover `address_parser.c` and `crf.c`; token payload/capacity counts come from dedicated `tokens.c`/container hooks. These are requested bytes and logical copy/view operations, not allocator metadata, physical writes, or per-token heap allocations. Normalized-address copying remains common.

## Work and correctness evidence

### All 13 observed datasets

The paired diagnostic capture is `raw/token-storage-diagnostics-v2/all13-diagnostic-v1/batch.json`. It covered 128,540 inputs. Candidate and control outputs passed byte comparison to the pinned original/golden expectations, and all parser thread samples were one.

- CPI copied 688,817 retained tokens and 3,819,000 payload bytes.
- 199 represented 616,106 tokens as views (3,531,981 referenced bytes) and copied 72,711 fallback tokens (287,019 bytes). The view path therefore avoided 616,106 payload-copy operations / 3,531,981 requested bytes in this instrumentation scope; it is not a zero-copy parse.
- 119,485 addresses were eligible; 9,055 took fallback. Opportunity and actual counters reconcile.
- Both variants had the same 3,350,544 instrumented search-API calls; 399,036 repeats; 1,930,029 materialized feature keys / 26,714,815 bytes; 5,530,942 model rows; 130,634,561 weight contributions; and 385,767 tracked address-parser/CRF allocation calls requesting 30,031,553 bytes. These counters do not show query/scoring work reduction.
- Maximum high-water capacity across the workload set did not fall: payload buffer capacity 256 B in each, offset and token-record capacities 64 entries in each, normalized buffer capacity 256 B in each. Maximum payload used bytes were 187 B for control and 139 B for candidate; normalized used maximum was 192 B in both. Summed growth events were 88 to 87 for the tracked char buffer (requested bytes 6,816 to 6,800); offsets and token records each had 20 events and identical requested growth bytes. Per-corpus maxima are not summed.

The focused 524-input guard-shape capture at `raw/token-storage-diagnostics-v2/guard-shapes-v1/` passed output and thread checks. It included development lines plus punctuation-adjacent spans, UTF-8, and a 240-character token. 490 addresses were eligible and 34 used the fallback; opportunity and actual fallback counts reconciled. This supplements, but does not exhaust, every possible span relationship.

### Broader correctness checks

The frozen safe API registry contains 78 callable records; four unsafe byte records were excluded. Native original, CPI, and 199 captures have 78 serialized ordered-output rows, identical byte-stream SHA-256 `02a8683c…`, and one observed parser thread per role. One record passes a null address pointer; embedded-NUL and nullable option semantics are preserved by the harness. A fixed-seed fuzz run emitted 200,000 rows and byte-compared successfully (SHA-256 `d9331f…`); 178,955 valid UTF-8 inputs were parsed, while 21,045 invalid UTF-8 inputs were skipped. Do not describe this as 200,000 parsed inputs.

The all-13 release preservation evidence and the above API/fuzz checks concern this candidate and retained exact artifacts. Unsafe API bytes and the rest of the input space remain outside these finite checks.

## Uninstrumented golden-v1 screen

Corrected locked run: `research/suite/results/201-token-storage-matched-golden-screen-v2/`. It ran the existing suite with 3 rounds × 200 passes over the 12,863-row golden-v1 dataset, roles original 000, packaged CPI, source-matched CPI, and 199. The 128,540-row figure belongs to the aggregate of all 13 diagnostic datasets, not this screen. All 12 timing roles exited 0. Memory and threads stages completed. The suite’s final Markdown rendering failed after writing `results.json`: `run_suite.py:377` applied an integer comma format to a missing dataset address count (`ValueError: Cannot specify ',' with 's'`). Preserve this report-stage failure; measurements are available in `results.json`, logs, and per-role captures. The earlier unprotected run attempt remains separately preserved as interrupted and is not included.

| Role | Warm median per address | Range across rounds | Cold median | Max peak footprint | Steady footprint after 3 passes |
|---|---:|---:|---:|---:|---:|
| Original 000 | 32.208 µs | 32.201–32.297 µs | 40.944 µs | 2,612,709,584 B | 2172 MB |
| Packaged CPI (PGO, scanner O1) | 5.286 µs | 5.219–5.544 µs | 8.608 µs | 2,537,343,352 B | 1927 MB |
| Source-matched CPI (no PGO, scanner O0) | 5.604 µs | 5.560–6.301 µs | 9.099 µs | 2,537,343,352 B | 1927 MB |
| 199 (same profile as matched CPI) | 5.558 µs | 5.496–5.564 µs | 8.497 µs | 2,537,359,736 B | 1927 MB |

The pooled warm medians put 199 0.8% below source-matched CPI (5.558 vs 5.604 µs) and 5.1% above packaged CPI (5.558 vs 5.286 µs). The three paired round medians for source-matched CPI vs 199 were 5.605/5.498, 5.560/5.5655, and 6.3095/5.5595 µs/address. The last matched-CPI round coincided with higher system activity: load average 3.35, with AppleNeuralEngine and replayd visible in the per-run process sample. Overall captured loads rose from 2.37 at settling to 3.90. This is a three-round, one-corpus screen under variable background load; the small pooled difference is not a stable speedup claim. 199’s peak footprint is 16,384 B above source-matched CPI and 75,349,848 B below original. Steady footprint is reported only to rounded MB, so equality to CPI and lower value than original do not establish a byte-precision memory gate.

The native API harness’s maximum across three captures was 2,612,283,600 B for original, 2,517,469,560 B for matched CPI+mimalloc, and 2,517,485,968 B for 199+mimalloc. Candidate was 16,408 B above matched CPI maximum, and 94,797,632 B below original maximum. This is whole-process C API memory, not parser-only footprint; the original capture range was 136.7 MB. The API result therefore remains a small candidate-vs-CPI difference under host/setup variability, not a blanket memory conclusion.

All six thread samples per suite role were exactly one. AC/environment gate passed at run start; the run was held under `WORKLOAD.lock`. The suite integrity stage was not selected. After the run, a metadata-only identity check pinned all four `bench_ext` hashes, the config, input TSV, frozen reference, and model manifest in `raw/suite-report-fix-v1/postrun-integrity.json`; this check invoked no parser. Its `dataset.rows` field incorrectly used the all-13 total (128,540). The configured screen inputs each contain 12,863 rows, matching the timing output's `corpus=12863`; the correction is preserved in `raw/suite-report-fix-v1/postrun-integrity-correction-v1.json`. No thermal/power gate was bypassed.

## Failures and remaining limits

- `raw/timing-screen-v1/` records the first suite launch interrupted after the locking-protocol error; it is not evidence.
- The corrected screen retained all timing/memory/thread data but failed only while formatting the final Markdown report. `research/suite/run_suite.py` was patched to display a missing manifest address count as an em dash, and to allow recovered output at a new path. Offline checks for missing, zero, 12,863, and boolean counts passed; the recovered report is `raw/suite-report-fix-v1/report_recovered.md`. Original source SHA-256 was `ac07848b1adcb7d403998a3f4615d144ed86cc88e9086d320e0c01ce96e86652`; patched source SHA-256 is `a5441ef9e16da24bca53c84ae837cdab93709ab324a1a293d03613a87dd41bb4`, with exact diff in `raw/suite-report-fix-v1/source.patch`. The saved pre-patch source and offline check record are also in that directory. No parser rerun was needed.
- The initial 201 diagnostic base hash was wrong: `98275…` identified the 199 diagnostic archive, not the CPI185 base. It was superseded with preserved correction in `raw/token-storage-diagnostics-v2/HANDOFF-v2.*`; the built pair verifies CPI base `c0700a8b…`.
- Native API harness had two pre-parser setup failures; those are retained and excluded from successful capture counts.
- Memory and timing results are limited to the declared golden dataset/configuration. No full 5×20 suite promotion campaign or profiler-based universal peak result was run.

## Conclusion

199 removes a substantial, directly counted token-payload copying workload on these inputs while retaining original fallback behavior and exact observable output. It does not reduce the measured search/model/weight work or maximum retained container capacity. The bounded release screen shows performance close to the matched CPI profile, not better than packaged CPI; memory is below 000 on this screen but rounded steady equality and API variability prevent a blanket acceptance statement. Keep the candidate as a measured research result; do not promote it on these observations alone.
