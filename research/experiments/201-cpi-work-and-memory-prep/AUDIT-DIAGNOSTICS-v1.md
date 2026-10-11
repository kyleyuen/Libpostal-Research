# Experiment 201 diagnostics evidence audit

Audit timestamp: 2026-10-09 America/Los_Angeles  
Scope: read-only audit of the completed all-13 paired diagnostic capture, binary/build identities, output/thread/frozen gates, and token-storage counter meanings. No builds or workloads were run by this audit.

## Evidence checked

- Batch record: `raw/token-storage-diagnostics-v2/all13-diagnostic-v1/batch.json`.
- Per-role result records: each dataset's `199-diagnostic/supplement.json` and `matched-cpi-diagnostic/supplement.json`, `capture/metrics.json`, `capture/threads.json`, and `gate/` files under that same directory.
- Diagnostic binary identities: `artifacts/token-diagnostics-v4/{control,candidate}/hashes.json`.
- Source counter implementation and definitions: token-storage diagnostic source patch/handoff v2, `metrics.json` definitions, and `research/tools/run_parallel_metric_supplement.py` result checks.

The build hash records name the same CPI185 base archive SHA-256 (`c0700a8b99e0f575fe98d8af913435c1d18e24d7896ef98e7e4f91dcc5684b56`), the same source manifest and common compile flags, and the same switches manifest. The candidate differs by `-DEXP_TOKEN_SLICES`; the control has no mode delta. The matched control is therefore a diagnostic build with token slices disabled, not a release binary. Earlier handoff v1 archive attribution was corrected in handoff v2; v1 is not used as provenance here.

## Gate audit

The batch contains 26 completed records: 13 candidate and 13 matched-control captures, with all 13 datasets represented once per role. Every record has exit code 0, status `pass`, and an empty stderr. All 26 retained-original comparisons are true. Every observed thread count is exactly 1. Within each corpus, candidate and control stdout SHA-256 values are identical.

Both golden-v1 roles additionally pass the frozen ordered-component comparison: 12,863 reference and dump rows, 12,863 unique identifiers/addresses, zero missing rows, zero ordered-component mismatches, `identical: true`. Other corpora compare exactly against their retained-original outputs; their supplement records correctly leave the golden frozen-reference fields null.

The raw output equality evidence supports the experiment's byte-output gate for these captured inputs. It is finite corpus coverage, not a proof for all possible inputs.

## Counter reconciliation

Totals below sum the 13 `metrics.json` records per role (128,540 input addresses and 688,817 emitted tokens in each role).

| Metric | Matched CPI control | 199 candidate |
|---|---:|---:|
| Retained-token copy calls | 688,817 | 72,711 |
| Retained-token copied payload bytes (excludes NUL) | 3,819,000 B | 287,019 B |
| Token view records | 0 | 616,106 |
| Token payload bytes represented by views | 0 B | 3,531,981 B |
| Copied-token NUL writes | 688,817 | 72,711 |
| View-end NUL writes | 0 | 616,106 |
| Strict-gap eligible input addresses (opportunity counter) | 119,485 | 119,485 |
| Unsafe input addresses requiring candidate fallback | 9,055 | 9,055 |

The counters reconcile exactly, in aggregate and independently for each dataset:

- Candidate copied tokens + view records = control copied tokens = 688,817.
- Candidate copied payload bytes + view-referenced payload bytes = control copied payload bytes = 3,819,000 B.
- Candidate copied-token NUL writes + view-end NUL writes = control copied-token NUL writes = 688,817.
- The two opportunity counters agree in both roles; candidate actual view/fallback address counts equal those opportunities.
- Normalized-text copy counters match between roles, per corpus and in aggregate: 128,540 calls, 4,482,638 payload bytes, and 128,540 NUL writes.

Observed char-buffer growth was 88 events / 6,816 requested bytes in the control and 87 / 6,800 requested bytes in the candidate. Offset-vector growth matched in both roles at 20 events / 1,856 requested bytes; token-record vector growth matched at 20 / 11,136 bytes. These are source-instrumented growth requests, not allocator-call totals across the full library.

### Counter definitions and limits

- `token_copy_calls` counts retained-token append/copy operations, not heap allocations. `token_copy_bytes` counts token payload bytes only, excluding NULs and normalized-address copies.
- `token_view_records` and `token_view_bytes` count retained token references and their payload lengths; the bytes describe referenced source data and avoided token-payload copies, not allocation savings or RSS.
- `token_copy_nul_writes` and `token_view_nul_writes` are counted separately because the copy path terminates each copied token while the view path writes NUL at safe source gaps.
- `token_slice_opportunity_*` counts strict-gap eligible and unsafe input addresses in both roles. `token_slice_eligible_addresses` and `token_slice_unsafe_fallbacks` report the candidate's actual decisions and are zero in the control by design.
- Growth event/request counters cover the instrumented token payload, normalized text, token offset vector, and token record vector growth sites. Requested-byte units are bytes. They do not mean total allocations or resident/physical memory.
- Per-process high-water counters with `_peak_bytes` are bytes; counters with `_peak_entries` are vector entries. Taking the maximum across the 13 process captures gives:

| Per-process high-water maximum across corpora | Matched CPI control | 199 candidate |
|---|---:|---:|
| Token payload used / capacity | 187 / 256 B | 139 / 256 B |
| Token offsets used / capacity | 33 / 64 entries | 33 / 64 entries |
| Token records used / capacity | 33 / 64 entries | 33 / 64 entries |
| Normalized text used / capacity | 192 / 256 B | 192 / 256 B |

These are the maximum values of reusable parser buffers within an individual capture, not corpus-summed storage, process RSS, or API peak memory. Generic `allocation_calls`/`allocation_requested_bytes` metrics cover only the explicitly instrumented `address_parser.c` and `crf.c` source bodies and exclude other translation units, including token/string helpers. They cannot establish a library-wide allocation reduction. A lower copy counter is not, by itself, a measured speedup.

## Counter self-checks and unsafe-shape coverage

The recovered small calibration record (`raw/token-storage-diagnostics-v2/calibration-small-v1/recovered-analysis-v1.json`) marks all copy/view, byte, NUL, opportunity, and normalized-copy identities true. Its original, CPI-control release/diagnostic, and candidate release/diagnostic outputs all match on four hand-authored addresses and are single-threaded. The first calibration wrapper raised an `AttributeError` after all parser roles finished; analysis was recovered from retained captures without rerunning the parser. That initial wrapper failure remains in the raw record.

The separate 524-row guard-shape capture exercises unsafe adjacency/fallback inputs. Its candidate reports 490 eligible and 34 unsafe addresses, 2,751 token views, and 342 copied fallback tokens; the full output is byte-identical to original and the role is single-threaded. The all-13 run independently exercises 9,055 unsafe fallback addresses across its 13 datasets. This verifies observed fallback use on the covered cases, but does not establish universal span safety beyond those inputs and the source guard review.

## Decision and remaining evidence

The diagnostics support a narrow conclusion: on these inputs the candidate represented 3,531,981 token payload bytes as views and copied 287,019 bytes for fallback, with exactly reconciled token counts, bytes, NUL writes, normalized-copy behavior, byte-identical output, and single-thread execution against the matched diagnostic CPI control.

No speed result is claimed from these diagnostic runs. Their `time -l` records are command/resource provenance, not the authorized timing comparison. The original packaged CPI and fixed-199 absolute comparators remain distinct from this matched-control attribution pair.

The native API memory comparison is outside this audit and remains unresolved until its harness results are captured and audited. Do not make a blanket API-memory acceptance claim or promote on the basis of these diagnostic counters. Any higher API peak remains a hard unresolved gate even if a separate release-corpus footprint is below the original.
