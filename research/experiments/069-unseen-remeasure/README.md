# Experiment 069 — re-measure on a dataset the code has never seen (golden-v4-osm-unseen)

Date/time/timezone: created 2026-10-02T22:55:48-0700; run (2026-10-02 evening) 2026-10-02T23:52:29-0700 → 2026-10-02T23:56:52-0700, America/Los_Angeles.
Status: complete — **gains generalize**

User request (2026-10-03): after a faster candidate, re-measure on data the code has not seen, to rule out overfitting. Dataset: `profiling/golden-v4-osm-unseen` (10,293 addresses, 42 countries, 45% non-ASCII; 56 cities absent from every earlier set; no exact overlap with golden-v1/novel/golden-v3). Command `caffeinate -i /bin/bash tools/run.sh` (all steps exit 0, `raw/run.log`).

Outputs (`raw/output_check.txt`): `va`, `lf` and `pl` are byte-identical to the original library (017's `dump_outputs_baseline`) on all 10,293 addresses.

Speed (sequential, 5 rotating rounds, 20 passes, paired with the original; `raw/analyze.stdout`):
    | binary | unseen µs/address | speedup vs original (range) | max peak footprint |
    |---|---:|---:|---:|
    | orig | 20.499 | 1.000× (1.00–1.00) | 2.612 GB |
    | va | 4.376 | 4.684× (4.64–4.74) | 2.370 GB |
    | lf | 4.130 | 4.964× (4.91–5.00) | 2.543 GB |
    | pl | 4.087 | 5.016× (5.00–5.05) | 2.543 GB |

Interpretation: on unseen data the deliverable `lf` is 4.96× and the stack head `pl` 5.02×, at or above their speedups on the screening sets (golden-v1 4.92×, OSM 4.91× for `lf`), so the improvements do not depend on the datasets used during development.
