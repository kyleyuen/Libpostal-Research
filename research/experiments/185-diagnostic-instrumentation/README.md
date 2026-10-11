# Experiment 185 — diagnostic instrumentation

Date: 2026-10-07, America/Los_Angeles. Record opened before source changes.
Status: diagnostic collection complete; no performance candidate or speed claim.

## Question and scope

Measure actual lookup calls, hit/miss results, exact within-address repeated queries, and selected parser/CRF work in the original, cpi, and prototype181 variants. The hypothesis was that call-site counters and collision-safe query identity would distinguish eliminated searches from work moved between pipeline stages. This experiment measured operations; it did not predict or measure a speedup.

Repeated-query identity during tracking is dictionary instance + operation semantics + exact query bytes, scoped to an address. Exported bounded pattern examples aggregate by semantic family + operation + key bytes, so they may merge distinct dictionary instances; see `COVERAGE.md`. Retained patterns are first-encounter examples, not most frequent patterns. No raw address or query bytes are emitted.

## Completed run

- Full collection: [`results/full13-retry3/REPORT.md`](results/full13-retry3/REPORT.md), machine-readable [`results.json`](results/full13-retry3/results.json), and run manifest in the same directory.
- Build set: `20261007-1625-pattern-summary-fix`; exact source, settings, artifact and model/input hashes are recorded in the manifest and build provenance.
- Window: 2026-10-07 15:35:58.782 PDT to 15:46:24.355 PDT; 10m25.57s. All 39 dataset/variant records completed with exit status 0; 13 datasets x 3 variants, no runtime errors.
- The 13 datasets were golden-v1, novel, osm-v3, and unseen-v4 through unseen-v13. All release and diagnostic output hashes matched the protected original outputs; all observed parsing thread counts were one. Golden-v1's six output/reference checks found 12,863 rows, zero missing IDs and zero ordered-component mismatches.
- This was a diagnostic run. Process durations include instrumentation overhead and are provenance only; the report explicitly says final benchmark not run. It makes no speed, RSS, or memory-improvement claim and promotes no candidate.

### Selected golden-v1 counters

| Variant | Lookup calls | Hits | Misses | Repeated queries | Repeats/address | Calls/address | lookup calls vs original |
|---|---:|---:|---:|---:|---:|---:|---:|
| original | 1,014,421 | 765,791 | 248,630 | 138,182 | 10.74 | 78.86 | 1.00x |
| cpi | 380,614 | 234,076 | 146,538 | 49,336 | 3.84 | 29.59 | 0.375x |
| prototype181 | 1,058,689 | 787,760 | 270,929 | 142,241 | 11.06 | 82.30 | 1.044x |

Prototype181 shifted 149,532 calls (14.1% of its total) into `context_fill`; original and cpi record no calls in that stage. These counters characterize instrumented sites, not a global count of every logical request: cpi's `EXP_CP_IDS` and `EXP_ROLE_IDS` precomputed-ID paths are not fully counted as direct logical requests. The full per-corpus stage/family tables and metric boundaries are in the report and [`COVERAGE.md`](COVERAGE.md).

Repeated-pattern retention is bounded at 128 examples per report. Golden-v1 summaries report all tracked repeat observations, with zero address-tracker overflow: original 225,321 observations / 87,139 groups; cpi 91,674 / 42,338; prototype181 233,397 / 91,156. Unknown/inapplicable metrics remain N/A rather than zero. Allocation counters cover direct parser/CRF malloc/calloc/realloc requests only, not total heap growth, RSS, or temporary-memory high-water.

## Verification and known limitations

Calibration fixtures verified exact-key repeats, dictionary separation, stage/family restoration, address reset, bounded pattern/query-table behavior and summary reconciliation. The latest calibration is under `raw/calibration-20261007-pattern-summary-fix/`; the corrected golden-v1 diagnostic sidecar/output validation is under `raw/golden-v1-original-validation-20261007-pattern-summary-fix/`.

Earlier failures are retained, not overwritten:

- `full13-retry1` was stopped after review found mutable stage/family/operation getter pointers caused scope-label leakage; stable interned labels were added and recalibrated.
- `full13-retry2` failed original metrics validation because omitted repeat groups' later observations were not included in the summary total; every record was rejected. The accounting fix was calibrated before retry 3.
- Initial cpi builds failed with incomplete production switches, and an initial diagnostic dumper link omitted its sidecar define. All command logs and failed artifacts remain under `raw/`; retry 3 used the corrected build set.

Metric gaps and exact source boundaries are enumerated in `COVERAGE.md`. Pattern examples can merge dictionary instances in the exported summary, and cpi precomputed-ID request counts are incomplete. Do not interpret lookup count reductions as measured speed gains or as complete counts of all logical requests.

## Decision

Experiment 185 establishes instrumented operation counts and verified output preservation across the 13 configured datasets. It does not meet the professor-facing timing/performance protocol and does not change the current deliverable or its claims. Any optimization based on these counts needs its own numbered experiment, then the established sequential timing and memory protocol.
