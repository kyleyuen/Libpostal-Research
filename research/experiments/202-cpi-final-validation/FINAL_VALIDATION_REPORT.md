# Experiment 202 — accepted CPI final validation

Date: 2026-10-10. Status: complete for the declared 14-dataset validation set.
The artifact tested is the accepted packaged experiment 119 CPI against the
preserved original 000. Experiment 202 made no parser source or binary changes.

## Result

The CPI output matched original output byte-for-byte on 130,341 inputs across
13 previously observed datasets and one separately generated, unused OSM
corpus (v14). Frozen golden-v1 and novel comparisons passed, and the intentional
mismatch control was detected. Every timing capture exited naturally with
status 0 and exceeded the protocol's 10-second parsing-time floor. Thread
samples observed a maximum of one parser thread in each role. The maximum peak
footprint across the 14 datasets was 2,543,405,432 B for CPI versus
2,613,905,616 B for original, a difference of -70,500,184 B. The Senzing
group remains a subgroup exception: its local CPI maximum was 58,982,640 B
higher than its original maximum. Steady footprints were sampled on one
representative corpus per timing group and are not per-dataset measurements.

Warm parsing speedups below use the **median of the three same-round paired
ratios** for each dataset, where each round's ratio is original warm median
µs/address divided by CPI warm median µs/address. The warm medians use passes
2 through the configured pass count; setup is excluded. The all-dataset
unweighted result is the geometric mean of the 14 per-dataset median ratios:
**5.806217×**. The source-weighted result first takes the geometric mean within
each of the three exact source labels, then gives the source groups equal
weight: **5.893289×**. The source groups are OpenStreetMap (12 datasets,
5.782694×), Senzing (golden-v1, 6.033570×), and Senzing-style (novel,
5.866359×). Raw durations were not pooled across datasets or cohorts, and no
combined confidence interval is claimed. The v14 generated suite report's
5.694× value is the geometric mean of its three round ratios; its paired-round
median used in the all-dataset summary is 5.715195×.

| Dataset | Source | Inputs | Original warm median (µs/address) | CPI warm median (µs/address) | Median paired ratio |
|---|---|---:|---:|---:|---:|
| golden-v1 | senzing | 12,863 | 31.453 | 5.213 | 6.033570× |
| novel | senzing-style | 12,863 | 31.825 | 5.425 | 5.866359× |
| osm-v3 | openstreetmap | 11,405 | 20.292 | 3.464 | 5.857968× |
| unseen-v4 | openstreetmap | 10,293 | 19.783 | 3.404 | 5.811692× |
| unseen-v5 | openstreetmap | 8,794 | 19.646 | 3.287 | 6.044923× |
| unseen-v6 | openstreetmap | 9,450 | 18.976 | 3.226 | 5.714927× |
| unseen-v7 | openstreetmap | 8,973 | 19.244 | 3.355 | 5.746022× |
| unseen-v8 | openstreetmap | 9,981 | 18.528 | 3.283 | 5.686698× |
| unseen-v9 | openstreetmap | 9,480 | 19.391 | 3.497 | 5.530804× |
| unseen-v10 | openstreetmap | 8,567 | 19.127 | 3.142 | 6.087524× |
| unseen-v11 | openstreetmap | 7,882 | 18.452 | 3.319 | 5.691548× |
| unseen-v12 | openstreetmap | 8,273 | 18.607 | 3.095 | 5.800187× |
| unseen-v13 | openstreetmap | 9,716 | 18.665 | 3.234 | 5.727328× |
| unseen-v14 | openstreetmap | 1,801 | 15.271 | 2.672 | 5.715195× |
| **Total** |  | **130,341** |  |  |  |

## Memory, startup and threads

| Timing group | Peak footprint: original (B) | Peak footprint: CPI (B) | CPI minus original (B) | Steady footprint: original / CPI |
|---|---:|---:|---:|---:|
| Senzing: golden-v1 + novel | 2,478,360,712 | 2,537,343,352 | +58,982,640 | 2172 / 1927 MB, golden-v1 only |
| OSM: v3–v13 | 2,613,905,616 | 2,543,405,432 | -70,500,184 | 2492 / 1926 MB, osm-v3 only |
| Fresh OSM: v14 | 2,611,349,712 | 2,517,322,104 | -94,027,608 | 2490 / 1926 MB, unseen-v14 only |
| **Across all 14** | **2,613,905,616** | **2,543,405,432** | **-70,500,184** | Representative samples only |

The global maximum is lower for CPI, while the Senzing subgroup comparison
regresses. Peak footprint includes setup and process startup; the captures do
not isolate which phase caused each high-water mark. Do not infer the source of
the subgroup difference from this result. CPI startup was consistently slower:
median setup was 11.70–11.72 seconds across groups versus 2.76–2.90 seconds
for original. Warm parsing ratios exclude that setup cost.

The Senzing and OSM suite thread stages sampled golden-v1 and osm-v3
respectively; all six samples per role reported one thread. The v14 suite's
short thread stage included zeros after the process had exited. A separate
live-process v14 supplement sampled each role six times at three-second
intervals while the process remained active; all 12 samples reported one
thread. Its processes were intentionally stopped after the six samples.

## Cohorts and environment

All timing captures used original 000 and the accepted packaged CPI in the
same cohort, with three rotated rounds and serial execution:

| Run | Inputs | Rounds × passes | UTC window | Evidence |
|---|---:|---:|---|---|
| `202-final-senzing-v2` | golden-v1 + novel, 25,726 | 3 × 200 | 2026-10-10 04:52–05:05 | `research/suite/results/202-final-senzing-v2/` |
| `202-ac-osm-v3-v13-resumed-v1` | osm-v3, unseen-v4–v13, 102,814 | 3 × 420 | 2026-10-10 06:03–07:07 | `research/suite/results/202-ac-osm-v3-v13-resumed-v1/` |
| `202-ac-unseen-v14-resumed-v1` | unseen-v14, 1,801 | 3 × 2300 | 2026-10-10 07:07–07:13 | `research/suite/results/202-ac-unseen-v14-resumed-v1/` |

All recorded hard environment gates passed on AC power with no thermal warning.
The OSM and v14 runs reached their 30-second soft settle limit and continued
with load telemetry, as the protocol permits. OSM timing load ranged 2.56–19.43;
all captures were retained, including noisy periods. The v14 timing captures
recorded load 2.68–3.63. The Senzing cohort settled immediately at load 1.77.
These were not uniformly quiet sessions; per-run environment snapshots and
per-pass CPU, wall, and context-switch data are retained. A previous AC OSM
attempt stopped at 23/54 captures, and two battery preparations produced no
timing captures; all remain preserved and are excluded from this result.

Exact invocations, settings and artifact identities are recorded in each
run's `invocation.json`, `suite.json`, `manifest.json`, `run.log`, `gate.log`,
`env.txt`, and per-capture files. Senzing used the recorded config
`raw/senzing-timing-v1.json` with run id `202-final-senzing-v2`; the run id is
not evidence of a separate v2 config. OSM used `raw/osm-ac-resumed-v1.json` and
`raw/run_osm_ac_resumed_v1.sh`. V14 used `raw/v14-ac-resumed-v1.json` and
`raw/run_v14_ac_resumed_v1.sh`. The original/candidate `bench_ext`,
`dump_outputs`, and fuzz executable SHA-256 values and all seven model-file
hashes match across the three run manifests. V14 input TSV SHA-256 is
`c795d62b629390c7ddf01d416f0d030e0bd9f075cb33d514ce10792b5b05e977`.

## Correctness and API scope

The 13 observed corpora comprise 128,540 inputs. The offline comparator
compared complete CPI JSONL output files byte-for-byte with cached original
000 outputs; all 13 matched. V14 adds 1,801 inputs: fresh complete original and
CPI JSONL streams were byte-for-byte equal (277,911 bytes each). The frozen
golden-v1 and novel checks passed, and a deliberate output mutation was
rejected by the negative control. These finite datasets do not cover all
possible inputs.

The all-13 correctness run also compared two 2,000,000-case fuzz digest/status
streams (seeds 1 and 2) against frozen original-reference streams; both
matched. This is hash/status evidence, not a bytewise comparison of serialized
component output for each generated input. No fresh exact-serializer fuzz run
was made for accepted CPI. The prior 201 exact 200,000-row serializer run was
for candidate 199, not CPI.

Prior experiment 201 evidence covers 78 callable safe public API registry
cases, with four unsafe byte cases excluded. Its original, CPI and candidate
captures had identical serialized ordered-output streams and observed one
thread each. The CPI side used an archive-linked shared-library adaptation;
this is not a new 202 test and does not establish every public API/lifecycle
mode. A prior audit also recorded a feature-print API abort on an older CPI
build; experiment 158 repaired that feature-print path in its own build. The
78-case registry does not include a fresh feature-print comparison, so this
report does not claim that the API gap has been newly retested here.

## Provenance and retained history

The CPI is unchanged from the accepted experiment 119 package; the baseline is
the preserved experiment 000 artifact. There is no new parser patch. Exact
commands, exit codes, input/model/artifact hashes and all raw timing outputs
are retained in the run directories above. Corrected-report snapshots are in
`raw/final-report-snapshots-v1/`; the v14 generated report had generic wording
that implied a broader OSM group and reserved-set exclusion. The live v14
report has been corrected with this run's actual scope, while its original
snapshot remains unchanged. The final machine-readable provenance and its
hash manifest are in `raw/final-validation-provenance-v1.json` and
`raw/final-validation-provenance-v1.sha256`.

Supporting audits:

- `raw/osm-ac-resumed-final-audit-v1.md` (11-dataset OSM capture audit).
- `raw/final-independent-audit-v1.md` (84-capture and all-14 rollup audit).
- `raw/v14-thread-supplement-v1/manifest.json` (six live samples per v14 role).
- `raw/202-observed13-integrity-correctness-v1.bytewise.json` and
  `raw/202-fresh-v14-correctness-v1.bytewise.json` (bytewise output checks).

## Decision

The accepted CPI passes the declared all-14 output, single-thread, and global
maximum-footprint comparisons. The Senzing subgroup peak result is retained as
a real exception, and CPI setup remains substantially slower than original.
The result supports the reported warm parsing speedup on these measured
corpora; it is not a claim about unobserved inputs, exact-output fuzz coverage,
or every public API mode. The separate 201 token-span candidate remains
unpromoted; this validation does not measure it.
