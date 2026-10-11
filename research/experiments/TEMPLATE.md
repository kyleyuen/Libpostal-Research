# Experiment NNN — short description

Date/time/timezone:
Status: planned / running / accepted / rejected / inconclusive
Evidence type: research diagnostic / final benchmark / both
Protocol: METHOD.md; operation-based research policy effective after experiment 183

## Hypothesis and change

Measured bottleneck and why this change should help:
Exact implementation change:
Base commit and candidate commit (or saved binary patch, including untracked source):
Incremental comparison base, if different from original libpostal:

## Operation-based research diagnostics

Predicted changes in actual work (including added bookkeeping/storage):
Metric definitions: units, increment locations, scope/reset, coverage and denominators:
Diagnostic build identities and matching unchanged-source control:
Diagnostic versus uninstrumented output check; counter calibration/negative control:

| Metric / stage / feature family | Control raw total | Candidate raw total | Per-address/token rate | Raw evidence |
|---|---:|---:|---|---|
| — | — | — | — | — |

Lookup calls across the entire pipeline; repeats/hits/misses/reused answers:
Traversal/probes/comparisons; text construction and bytes moved:
Allocation calls/bytes/growth/temporary high-water; model rows/weights accessed:
Filter checks/searches avoided; fallback frequency and work:
Profile/hardware counters (separate session; availability and limitations):
Matching-build ablation and attribution:
Unavailable/inapplicable metrics and why:
Diagnostic overhead, operation tradeoffs and hypothesis supported/refuted/inconclusive:

## Reproduction

Hardware/OS/compiler:
Corpus/split/order and SHA-256:
Model hashes:
Build flags, exact commands and command-log path:
Harness and library hashes:
Warm-up, duration, trial order, other workload/thermal observations:

## All measured trials

Final benchmark status: not run / incomplete / complete. Use uninstrumented
builds; diagnostic timings/count reductions are not final speed or memory claims.

| Trial | Baseline or candidate | Addresses/s | CPU/wall seconds | Peak RSS bytes | Raw log |
|---|---|---:|---|---:|---|
| — | — | — | — | — | — |

Median throughput, spread and speedup:
Peak RSS difference (bytes and percent):
Peak footprint difference (bytes and percent; macOS acceptance metric):
Steady-state memory evidence or limitation:
Any excluded trials, original logs and reasons:

## Correctness and interpretation

Exact output comparison count, mismatches and log:
Additional tests for the changed code:
Profile evidence explaining the change (if collected):
Development result vs validation/final result:
Known limitations and remaining uncertainty:

## Decision

Accepted / rejected / inconclusive, with evidence:
Next action:
Ledger, report and activity log updated:

Never replace missing measurements with zero or projected gains. Retain failed
attempts and do not modify frozen references to make the candidate pass.
