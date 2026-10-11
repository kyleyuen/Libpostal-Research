# Experiment 200 — fused ASCII normalization/tokenization pipeline

Date/time/timezone: started 2026-10-07 19:24 PDT; v2 release built 2026-10-07 19:47 PDT; America/Los_Angeles.  
Status: runnable v2 implementation; corrected v9 development/golden diagnostics passed output and thread checks but showed only 24/12,863 (0.19%) golden eligibility, so this candidate remains stopped before all13. No speed or memory result is claimed.  
Evidence type: source implementation, model-free scanner helper check and successful release build.  
Protocol: `research/METHOD.md`; exact ordered output, one parsing thread, and no peak/steady footprint regression are hard gates.

## Hypothesis and change

Experiment 163 identified separate normalization and tokenization traversals. For inputs containing only ASCII letters, digits and spaces, this experiment drives the unchanged scanner over the original input once, copies/lowercases each scanner span into an output buffer, and records the scanner's exact span/type. Unsupported input or unexpected scanner behavior clears partial token state and uses the original normalizer plus tokenizer. This remains a deliberately narrow guarded path; it adds per-byte checks and may do attempted work before fallback.

The active source snapshot is `sources-v2/`; the exact patch from experiment 119 and source manifest are `raw/source-v2.patch` and `raw/source-v2-manifest.sha256`. V1 source, artifact and handoff remain retained under `sources-v1/`, `artifacts/release-v1/`, and `raw/handoff-v1*.json`. V1's cursor-equals-end guard incorrectly forced fallback because `scan_token()` advances past the terminator after the last token. V2 instead validates that token spans begin at the previous covered offset and cover the entire input. The v1 failure and debug logs remain under `raw/scanner-test-v1/` and `raw/scanner-test-v2-debug/`.

Base: experiment 119 source snapshot, commit `181737d30977adc9d287e3f7a1cfafd7277525d4`; comparison base: experiment 119 / CPI switch set. V2 is built with `EXP_FUSED_ASCII_PIPE`; no certifier or token-slices flag is present.

## Operation-based research diagnostics

No counters were added to the release. The predicted change is one scanner traversal that also lower/copies covered bytes, replacing separate normalization and tokenization traversals on eligible inputs. The added work is ASCII classification and copying per consumed byte. Fallback inputs perform an attempted scan before original normalization/tokenization. No cache, country/key list or corpus-dependent behavior was added.

| Metric / stage / feature family | Development raw total | Golden-v1 raw total | Per-address/token rate | Raw evidence |
|---|---:|---:|---|---|
| Corrected v9 aggregate and fused counters | Development: 304,875 lookups, 39,471 repeated queries, 483,550 model rows, 11,261,655 accumulated weights; 41,222 tracked allocation calls / 3,095,287 requested bytes; 10,308 attempts, 21 accepts, 10,287 domain fallbacks, 168,054 scanner-covered bytes, 131,618 fused-copy bytes | Golden-v1: 380,614 lookups, 49,336 repeated queries, 603,376 model rows, 14,045,912 accumulated weights; 51,439 tracked allocation calls / 3,857,657 requested bytes; 12,863 attempts, 24 accepts, 12,839 domain fallbacks, 209,630 scanner-covered bytes, 164,071 fused-copy bytes | 0.20% and 0.19% accepts | `research/analysis/parallel-exploration-2026-10-07/raw/{200-development,golden}-diagnostic-supplement-v9/capture/metrics.json` |
| Fallback work after the guarded scan | Development: 10,287 calls each to original normalize/tokenize; 401,720 / 401,684 input bytes; 10,287 reusable-buffer copies / 401,684 bytes; 60,596 token-copy calls / 343,149 bytes | Golden-v1: 12,839 calls each to original normalize/tokenize; 501,524 / 501,486 input bytes; 12,839 reusable-buffer copies / 501,486 bytes; 75,659 token-copy calls / 428,381 bytes | All observed fallbacks run the original path | Same v9 metrics files |

Every observed fallback in v9 was attributed to an outside-domain byte; the span, empty, allocation and invalid-token categories had no events. Spaces are scanner `WHITESPACE` spans that are copied and advance contiguous coverage before being omitted from emitted tokens, so whitespace is not the coverage failure. The source gate accepts only `[A-Za-z0-9 ]`; common address punctuation and non-ASCII bytes trigger fallback. On fallback the original normalizer and tokenizer run after the helper has allocated a `len+1` buffer and may have scanned/copied a prefix. Candidate aggregate allocation totals exceed CPI control by 10,287 calls / 412,007 requested bytes on development and 12,839 calls / 514,363 bytes on golden; each call delta equals the observed fallback count and is consistent with the added guarded-path allocation. The candidate also records 131,618 development and 164,071 golden bytes copied into its fused temporary buffer before later stages, including addresses that fall back. V9 source restores the 185 CPI lookup/stage/allocation hooks; generic counts match control for lookups, repeats, model rows and weights. The earlier v7 captures remain preserved as partial custom-counter evidence with incomplete generic hooks. The v9 fused counters and low coverage do not establish speed or memory improvement.

## Reproduction

Hardware/OS/compiler: Apple arm64 macOS; Apple clang from Xcode. Release library build uses `-O2 -flto`; scanner object uses `-O0`; link uses `-flto`; no PGO. Experiment 119/CPI flags plus `-DEXP_FUSED_ASCII_PIPE`; no `EXP_CERT_F32_VITERBI` or `EXP_TOKEN_SLICES`.

The v2 build ran under exclusive `research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock` through `research/tools/run_locked.py`; command and captured timestamps, stdout, stderr and exit code are `raw/build-v2.*`. Exit code: 0. Release library SHA-256: `f07af993a9ed278903514f4097fa59e4bdac71256acae492054d787e933cfe42`; dumper SHA-256: `27423c4fffc35e09ba2b07244336e686e6786f7caf004d11d8744f43bd5b2896`. Complete artifact/source/build identities are in `raw/handoff-v2.json`. Model manifest SHA-256: `1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad`.

The first restored-hook 199 diagnostic derivative link attempt used the prototype181 archive and failed on unresolved CPI symbols; its stderr is retained at `research/analysis/parallel-exploration-2026-10-07/raw/199-diag-build-v8/11--O2.stderr`. Corrected v9 used the canonical CPI diagnostic base archive at `research/experiments/185-diagnostic-instrumentation/artifacts/build-20261007-1625-pattern-summary-fix/cpi/diagnostic/libpostal.a`; build manifest is `research/analysis/parallel-exploration-2026-10-07/raw/200-diag-build-v9/build-manifest.json`. All three TUs (`address_parser.c`, `tokens.c`, `diag.c`) compiled with `EXP_FUSED_ASCII_PIPE`, CPI185 switches and `EXP185_DIAG`; source SHA-256s are in the manifest. Frozen derivative source manifest SHA-256: `c8474ba881543a2d23fe3b4744f1bc9fcd3030f787744f3ab355b1d042a0245e`. V9 diagnostic archive SHA-256: `37aa5756cdc63d81dd0d8b696d984e322b67c2e79de73c6e190cb84172709947`; dumper SHA-256: `d53b3e5d1c483bfbb28801ac498cb7a5110a1836feb6097a42e4183ab5c435c9`. The candidate `tokens.h` equals the CPI185 header; candidate and diagnostic headers are covered by the source manifest. This v9 diagnostic build does not replace or modify the v2 release artifact.
### Model-free helper validation

The test extracts the exact candidate helper, compares its normalized bytes and token span/type records with `tokenize(lower(input))`, and tests unsupported-input cleanup. It passed all strings of length 1–6 over `AaZz09 `, targeted case-sensitive scanner examples (`I`, `IV`, `XII`, ordinal forms, `1E2`, `St`, `PO`, `UK`, `NW`, mixed-case address phrases), a 4096-byte stress string, and empty/tab/punctuation/UTF-8 fallback cases. Test stdout and exit are in `raw/scanner-test-v2.stdout` and `.exit`; generated harness and test source are `raw/scanner-helper-test.c` and `tools/model_free_scanner_test.py`. The test linked the helper against the unchanged experiment-198 scanner object compiled at `-O0`, SHA-256 `06cfaf25be695731afa6e0b63a9f773a7d2a6e52c7315325767aeef6a442a381`.

The first attempt to compile the helper together with `scanner.c` at `-O2` was interrupted with exit 130 after approximately three minutes of single-core compilation. Its command and limitation are recorded in `raw/scanner-helper-compile-o2-interrupted.md`; its terminal output was not redirected and is incomplete. This was a helper compile only, not a parser/model run, and it was not counted as a successful build. The successful helper check reused the unchanged O0 object.

This helper test is not a parser/model golden test and does not prove universal parser equivalence. No corpus or model was opened for it.

## All measured trials

The corrected v9 development/golden diagnostic eligibility screen is complete; the benchmark coordinator stopped this candidate before all13 based on 24 accepted inputs out of 12,863 golden-v1 addresses. Uninstrumented speed and memory trials were not run, so this decision is based on low eligibility and added fallback work, not a measured speed regression.

| Trial | Baseline or candidate | Addresses/s | CPU/wall seconds | Peak RSS bytes | Raw log |
|---|---|---:|---|---:|---|
| — | — | — | — | — | — |

Throughput, spread, speedup, peak RSS, macOS peak footprint and steady-state memory: unmeasured. No parser trials have been excluded by this worker.

## Correctness and interpretation

The v9 diagnostic candidate matched retained original output on 10,308 development and 12,863 golden-v1 addresses and observed one parsing thread. Eligibility was 21/10,308 development and 24/12,863 golden-v1; every fallback was attributed to the current byte whitelist. The generic fallback protects rejected input; finite corpus comparisons do not prove universal equivalence. The source/header audit confirms v9 compiled candidate `address_parser.c`, `tokens.c`, and `diag.c`, with the `tokens.h` candidate header present in the include tree; the custom and restored 185 counter source hashes match the v9 build manifest. V9 archive SHA-256 is `37aa5756cdc63d81dd0d8b696d984e322b67c2e79de73c6e190cb84172709947`; dumper SHA-256 is `d53b3e5d1c483bfbb28801ac498cb7a5110a1836feb6097a42e4183ab5c435c9`. A bounded future direction is to consider printable ASCII except `&`, which existing `EXP_ASCII_NORM` source documents as normalizing by ASCII lowercasing, while retaining complete span validation and `INVALID_CHAR` fallback. Any broader fast-path equivalence would also require scanner raw-case/type checks. This is a research option only, not a source change or verified parser-screen result. V9 output checks do not provide complete runtime or memory evidence.

## Decision

Stopped as an all13 candidate after the corrected v9 eligibility screen: 24 of 12,863 golden-v1 addresses (0.19%) took the fused path, while all 12,839 others incurred a fallback. The v2 release build and model-free helper remain successful, and v9 development/golden outputs matched retained original outputs with one observed thread. Generic CPI operations match control; tracked allocation work is higher by 12,839 calls and 514,363 requested bytes on golden. No runtime or memory result is available, and no speedup claim is made. A bounded future option is documented above; it requires separate scanner/type and normalization equivalence evidence before implementation or promotion. Parent coordinator owns global ledger, activity log and professor report updates.
