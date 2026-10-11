# AC OSM resumed cohort: final timing audit

Audit time: 2026-10-10 07:08 UTC
Run: `202-ac-osm-v3-v13-resumed-v1`
Prior checkpoints: [`osm-ac-resumed-checkpoint-audit-v1.md`](osm-ac-resumed-checkpoint-audit-v1.md), [`osm-ac-resumed-checkpoint-audit-v2.md`](osm-ac-resumed-checkpoint-audit-v2.md).

This read-only audit covers the completed OSM timing, representative steady
memory, and representative thread stages. It does not combine canceled AC
captures or failed battery attempts with this cohort.

## Timing captures and environment

- The frozen run covers 11 OSM datasets, `osm-v3` and `unseen-v4` through
  `unseen-v13`, with original 000 and packaged CPI, three rounds, and 420
  passes per capture: 66 total captures. All 66 have exit code 0 and 420 pass
  records.
- Parsing-only duration was calculated from each capture's saved stdout as
  `sum(pass us_per_addr × corpus row count) / 1,000,000`. The row count is the
  `corpus=` value in the `setup_seconds=` header. This excludes setup time;
  `/usr/bin/time -l` real time includes setup and is not used for the 10-second
  floor. All 66 meet the floor. Minimum parsing time is 10.791333 s (unseen-v11
  CPI, round 3); maximum is 100.545876 s (osm-v3 original, round 3).
- The standard hard gate passed for every timing, memory, and thread stage
  launch: all 70 gate records say AC Power. The 66 per-capture environment
  snapshots contain no recorded thermal/performance warning. The initial
  settle wait reached its 30-second limit at load 3.73/4.24/4.56 and continued
  with telemetry, consistent with the protocol's soft load-settle rule.
- Per-capture load averages range 2.56–19.43. Nine captures reached load >=10;
  every trial remains included. Logs retain each pass's CPU and wall times and
  context switches, plus `/usr/bin/time -l` real/user/sys, RSS, peak footprint,
  instructions, and cycles.

## Pinned input and artifact identities

The integrity stage recorded 11 datasets / 102,814 rows and seven model files.
I rehashed all 28 path/hash entries in the suite manifest against the current
files: 22 dataset TXT/TSV files and the original/CPI `bench_ext`, `dump_outputs`,
and `fuzz_pipeline` executables. All 28 match. The selected model file hashes
are preserved in the run manifest. The original/CPI executables also match the
202 start-state hashes. No golden-v1 correctness claims are inferred from this
OSM timing stage.

## Independently checked OSM results

I independently recalculated each role's median over passes 2–420, paired the
three matching rounds, and divided original warm microseconds/address by CPI
warm microseconds/address. All 33 paired ratios agree with `results.json`.
Across the 11 OSM datasets, the suite's all-dataset geometric mean of the
per-dataset median round ratios is 5.788870× (reported 5.79×). Per-dataset
median round ratios range 5.530804×–6.087524×. Because all 11 datasets have the
same OpenStreetMap source, the source-weighted and unweighted OSM summaries are
identical. This is OSM-only; it is not the all-14 result.

The OSM peak-footprint maxima are 2,613,905,616 B for original 000
(`unseen-v8`, round 3) and 2,543,405,432 B for packaged CPI (`osm-v3`, round
3), a subgroup difference of -70,500,184 B under
`max(candidate peak) - max(original peak)`. This is the completed OSM subgroup
only. It does not erase the independently recorded Senzing subgroup increase
of 58,982,640 B, and it does not substitute for an all-14 global maximum.

The steady-footprint sample is `osm-v3` only, after three warm passes:
original 2492 MB and CPI 1926 MB, reported at rounded MB precision. The thread
stage also targets `osm-v3` only; six samples per role all observed one thread.
Neither representative stage establishes per-dataset coverage across all 11
OSM corpora.

## Evidence

- Frozen scope and hashes: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/invocation.json`, `manifest.json`, and `suite.json`.
- Capture order, exits, and load: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/run.log` and `timing/*.exit`.
- Gate and per-run environment records: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/gate.log` and `env.txt`.
- Per-pass data and resource logs: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/timing/*.stdout` and `timing/*.stderr`.
- Reported and structured calculations: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/report.md` and `results.json`.
- Representative steady and thread scope: `memory.json`, `threads.json`, and `run.log` in the same result directory.
