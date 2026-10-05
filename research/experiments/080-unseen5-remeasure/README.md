# Experiment 080 — re-measure of the new candidates on a second unseen set (golden-v5-osm-unseen)

Date/time/timezone: created 2026-10-03T03:07:38-0700; run 2026-10-03T03:07:45-0700 → 2026-10-03T03:13:17-0700, America/Los_Angeles. Status: complete

User rule: each faster candidate is re-measured on data the code has not seen. Dataset `profiling/golden-v5-osm-unseen` (8,794 addresses, 38 countries, 42% non-ASCII; cities disjoint from v3/v4; no exact overlap with v1/novel/v3/v4). Candidates batched: lf (validated deliverable), pf2 (074), pcp (077), ptp (079). Command `caffeinate -i /bin/bash tools/run.sh`; 25 timing runs exit 0.

Outputs (`raw/output_check.txt`): all four identical to the original library on all 8,794 addresses.
Speed (sequential, 5 rotating rounds, 20 passes; `raw/analyze.stdout`):
    | binary | unseen-v5 µs/address | speedup vs original (range) | vs previous row | max peak footprint |
    |---|---:|---:|---:|---:|
    | orig | 19.929 | 1.000× (1.00–1.00) |  | 2.613 GB |
    | lf | 4.027 | 4.938× (4.92–4.97) | 4.9378 | 2.538 GB |
    | pf2 | 3.975 | 5.016× (4.99–5.04) | 1.0148 | 2.538 GB |
    | pcp | 3.938 | 5.040× (5.03–5.10) | 1.0089 | 2.538 GB |
    | ptp | 3.968 | 5.025× (4.96–5.04) | 0.9937 | 2.538 GB |

Conclusion: pf2 (+1.5% over lf) and pcp (+0.9% over pf2) generalize to unseen data; **pcp 5.04×**. ptp loses 0.6% → 079 rejected.
