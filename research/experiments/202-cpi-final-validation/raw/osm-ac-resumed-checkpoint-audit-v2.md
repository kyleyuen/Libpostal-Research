# AC OSM resumed cohort: checkpoint audit v2

Audit time: 2026-10-10 06:48:15 UTC (2026-10-09 23:48:15 PDT)
Previous checkpoint: [`osm-ac-resumed-checkpoint-audit-v1.md`](osm-ac-resumed-checkpoint-audit-v1.md).

This is an additive, read-only checkpoint for `202-ac-osm-v3-v13-resumed-v1`.
No parser, model, build, or benchmark was run for this audit. Earlier canceled
and failed cohorts remain separately preserved.

## Scope and calculation

The frozen run selects 11 OSM datasets (`osm-v3`, `unseen-v4` through
`unseen-v13`), original 000 and packaged CPI, three rounds, and 420 passes per
capture: 66 planned timing captures. At this checkpoint, captures 1–22 complete
round one, captures 23–44 complete round two, and the `osm-v3` pair is complete
in round three (captures 45–46). Thus 46/66 captures are complete.

For each completed capture, parsing-only duration was recalculated from the
saved pass records:

```text
sum(pass us_per_addr × corpus row count) / 1,000,000
```

The corpus row count is the `corpus=` value in the `setup_seconds=` header.
Setup is excluded. `/usr/bin/time -l` real time includes setup and remains a
separate wall-time measurement; it does not determine whether a trial meets
the 10-second parsing floor.

## Checkpoint findings

- All 46 completed captures exit 0 and contain exactly 420 pass records. All
  meet the parsing-only floor; the shortest is unseen-v12 packaged CPI, round
  two, at 10.801667 seconds. The longest is osm-v3 original 000, round three,
  at 100.545876 seconds.
- There are 47 successful per-capture gate records at this checkpoint, all
  reporting AC Power. The environment snapshots report no thermal or
  performance warning. The standard gate checks Low Power Mode, pause files,
  and concurrent project workloads as well. One gate record is for the next
  capture and has no completed timing record yet.
- Load averages across completed captures range from 2.57 to 19.43. Nine
  captures recorded load >=10; all remain in the data. No trial was filtered
  based on load.
- `/usr/bin/time -l` peak-footprint observations so far range by role as
  follows: original 000, 2,276,198,464–2,612,840,656 B; packaged CPI,
  2,538,293,624–2,543,405,432 B. These are partial-cohort observations only;
  the global maximum comparison is not final until all declared eligible
  captures and required memory stages are complete. Do not infer acceptance
  from this checkpoint.
- Captures retain per-pass CPU and wall time and context switches, plus
  `/usr/bin/time -l` real/user/sys and resource counters. Load telemetry and
  every high-load trial are preserved for the final variability analysis.

## Final arithmetic workflow

1. Require the full OSM timing cohort (66/66), plus completed eligible Senzing
   and v14 groups. Keep each group's raw timing samples, power provenance, and
   load telemetry separate; do not pool raw per-address times across different
   corpora or across canceled/battery and final AC sessions.
2. For each dataset, derive each role's per-round warm median from passes 2–420.
   Pair matching rounds and compute `original warm median us/address / CPI warm
   median us/address` (equivalent to the throughput ratio for that paired work).
   Report the geometric mean and 95% t interval on log per-round ratios, plus
   the median and min–max across rounds, matching `research/suite/run_suite.py`.
   For a multi-group summary, retain each dataset's median per-round ratio; show
   the suite's source-weighted geometric mean and the unweighted geometric mean
   across datasets separately. State both denominators. Do not pool unlike raw
   timings or claim one pooled CI across separate corpora/sessions.
3. For the declared all-14 peak-footprint gate, compare the largest eligible
   candidate peak with the largest eligible original peak across the same 14
   declared datasets: `max(candidate peak footprint) - max(original peak
   footprint)`. Include the completed eligible Senzing, OSM, and v14 groups;
   exclude canceled and failed battery attempts. Report the Senzing subgroup
   regression separately, even if another dataset sets the global maximum.
4. Judge steady footprint and thread counts using their declared representative
   datasets and captures. Do not describe one representative steady/thread
   check as coverage across all 14 datasets.

## Evidence paths

- Frozen scope/config: `research/experiments/202-cpi-final-validation/raw/osm-ac-resumed-v1.json` and `research/suite/results/202-ac-osm-v3-v13-resumed-v1/invocation.json`.
- Capture order, exit status, and load: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/run.log` and `timing/*.exit`.
- AC gates and environment snapshots: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/gate.log` and `env.txt`.
- Pass-level timing/setup/corpus: `research/suite/results/202-ac-osm-v3-v13-resumed-v1/timing/*.stdout`.
- Peak footprint, wall/user/system time, context switches, and resource counters: matching `timing/*.stderr` files.

This checkpoint supersedes v1 only for the later capture count and updated
metrics. It is not the final campaign result.
