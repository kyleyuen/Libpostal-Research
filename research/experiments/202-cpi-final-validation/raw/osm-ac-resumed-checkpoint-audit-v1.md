# AC OSM resumed cohort: checkpoint audit

Audit time: 2026-10-10 06:33:28 UTC (2026-10-09 23:33:28 PDT)

This is a read-only checkpoint for `202-ac-osm-v3-v13-resumed-v1`. It preserves
the prior cancelled-cohort and battery-attempt records. No parser, model, build,
or benchmark was run for this audit.

## Scope and calculation

The frozen invocation selects 11 OSM datasets (`osm-v3`, `unseen-v4` through
`unseen-v13`), with original 000 and packaged CPI, three rounds, and 420 passes
per capture: 66 planned timing captures. This checkpoint covers the first 30
captures logged by the suite, through round-two `unseen-v6` CPI. All 30 have
exit code 0, and each stdout contains 420 pass records.

For each completed capture, parsing-only duration was calculated from its saved
stdout as:

```text
sum(pass us_per_addr × corpus row count) / 1,000,000
```

The row count is the `corpus=` value in the `setup_seconds=` header. Setup time
is excluded. `/usr/bin/time -l` real time includes setup and is retained as a
separate wall-time field; it is not used to determine the 10-second parsing
floor.

## Checkpoint findings

- All 30 completed captures exited 0, contained 420 passes, and met the
  10-second parsing-only floor. The shortest was `unseen-v11` packaged CPI
  round 1 at 11.148222 seconds; the longest was `osm-v3` original 000 round 1
  at 99.020936 seconds.
- There are 31 successful per-capture gate records at this checkpoint, each
  reporting AC Power. The recorded environment snapshots report no thermal or
  performance warning. The standard gate also checks Low Power Mode, pause
  files, and concurrent project workloads. One gate entry is for the next
  capture, which had not yet appeared in the completed-capture log at audit
  time.
- Recorded load averages across the 30 completed captures range from 2.57 to
  19.43. The captures at load 12.37, 12.98, 13.58, 17.20, 19.43, 12.03,
  11.08, 15.01, and 12.24 remain included. No trial was filtered by load.
  The suite recorded a soft-settle timeout before timing and continued with
  load telemetry, consistent with the method's hard environment gates.
- Per-capture stdout/stderr retain per-pass wall and CPU time, context switches,
  `/usr/bin/time -l` real/user/sys durations, and resource counters. These
  remain available for later robustness review; this checkpoint makes no
  speedup conclusion.

## Evidence paths

- Invocation and exact selected scope: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/invocation.json` and `suite.json`.
- Capture exits, order, and load telemetry: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/run.log` and `timing/*.exit`.
- Gate records and environment snapshots: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/gate.log` and `env.txt`.
- Per-pass values and setup/corpus headers: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/timing/*.stdout`.
- Wall/CPU/resource/context-switch records: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/timing/*.stderr`.

This is an interim audit only. Later captures require the same per-pass duration
calculation and gate/exit review before final cohort summaries are written.
