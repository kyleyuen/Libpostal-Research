# Experiment 203 steady-memory samples: independent audit

Audit scope: saved steady-memory artifacts for golden-v1, OSM v3, and unseen-v14. No workload or build was run for this review. Thread-count evidence remains separate.

## Sample validity and identity

Each run contains an invocation limited to the `memory` stage, a run log, two AC gate records, two raw `footprint -p` outputs, two `bench_ext` stdout captures, and a `memory.json`. Each raw footprint file reports a named `bench_ext` PID and a nonzero `Footprint` value. The value and unit match `memory.json`; none of the six samples is missing, zero, or expired. The runner waits until it observes pass 3, invokes `footprint -p` on the still-running process PID, then kills and waits for that process. The captured stdout continues beyond the readiness threshold; observed last passes were golden-v1 44/45 (original/candidate), OSM v3 19/19, and v14 37/81. Those last logged pass numbers are not a claim that sampling occurred exactly on that pass.

All six candidate stdout files contain the active streaming marker. The steady-run configs use the same original/candidate executable paths and model directory as the timing cohorts. I verified both executable hashes against the timing manifest and all seven current model-file hashes against its model manifest; all matched.

## Recorded representative steady footprints

| Dataset | Original | Candidate | Candidate minus original |
|---|---:|---:|---:|
| golden-v1 | 2,492 MB | 1,927 MB | -565 MB |
| osm-v3 | 2,492 MB | 1,926 MB | -566 MB |
| unseen-v14 | 2,490 MB | 1,926 MB | -564 MB |

Each run records two successful AC gates and the corresponding original/candidate samples. The raw footprint headers show the same rounded values as the JSON records.

## Scope

These are representative live steady-footprint samples after the runner observed at least pass 3; they are separate from peak footprint and RSS values in the timing captures. They do not offset or replace any peak result. This memo makes no thread-count claim and does not establish whole-experiment 203 acceptance; live-thread evidence remains to be audited, and the historical 202 peak-memory failures remain part of the record.
