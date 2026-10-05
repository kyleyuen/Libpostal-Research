# Experiment NNN — short description

Date/time/timezone:
Status: planned / running / accepted / rejected / inconclusive

## Hypothesis and change

Measured bottleneck and why this change should help:
Exact implementation change:
Base commit and candidate commit (or saved binary patch, including untracked source):
Incremental comparison base, if different from original libpostal:

## Reproduction

Hardware/OS/compiler:
Corpus/split/order and SHA-256:
Model hashes:
Build flags, exact commands and command-log path:
Harness and library hashes:
Warm-up, duration, trial order, other workload/thermal observations:

## All measured trials

| Trial | Baseline or candidate | Addresses/s | CPU/wall seconds | Peak RSS bytes | Raw log |
|---|---|---:|---|---:|---|
| — | — | — | — | — | — |

Median throughput, spread and speedup:
Peak RSS difference (bytes and percent):
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
