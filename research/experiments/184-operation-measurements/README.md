# Experiment 184 — operation measurement runner preparation

Date/time/timezone: 2026-10-07, America/Los_Angeles.
Status: prepared, not run. Evidence type: research diagnostic tooling.
No parser source changes, builds, measurements or tests executed.

## Hypothesis and change

Comparable operation counts can distinguish reduced work from work moved between
stages. Created `../../tools/run_measurements.py` and `config.json` using Python's
standard library. All 13 existing datasets are selected; all are spent regression
sets. No claim of speed, memory improvement or successful execution.

## Reproduction, once prepared and on AC

From the workspace root, these are future commands, **not executed**:

```sh
python3 research/tools/run_measurements.py
python3 research/experiments/184-operation-measurements/test_runner.py
python3 research/tools/run_measurements.py --run
```

The first command only reports configuration readiness. `--run` executes serial
diagnostics, with the existing AC/thermal/concurrent-process gate before every
process. It never builds binaries, never calls the speed benchmark and has no
gate bypass. A new result directory preserves commands, start/end times, exit
codes, stdout/stderr, thread samples and file hashes. Failed attempts are retained.
Existing result paths cannot be overwritten.

**Preparation still required:** the three `diagnostic_dump` and `provenance`
entries are deliberately null. No current release dumper emits this full metric
schema. The runner refuses to launch workloads until those entries and files
exist. This runner is the script requested; parser instrumentation is separate
unfinished work, not implied by writing it.

## Diagnostic dumper interface

Each separately built diagnostic dumper accepts one TXT path, prints exactly the
existing dumper JSONL to stdout, and writes aggregate JSON to the absolute path
provided by `MEASUREMENTS_JSON`. No shell commands are accepted in the config.
The sibling release dumper must use the same source and build settings except
instrumentation. The original derivative must remain separate from the preserved
000 library. `provenance` names a saved record of source hashes/patch, compiler,
flags, linked library/model, metric coverage and increment locations. The runner
hashes that record; its truth must also be reviewed before accepting results.

Example metric payload (illustrative values, not measured):

```json
{
  "schema_version": 1,
  "scope": "parsing",
  "addresses": 2,
  "tokens": 8,
  "counters": {
    "lookup_calls": {
      "value": 30,
      "definition": "v1: actual dictionary/model search invocations across every parsing stage; excludes setup"
    },
    "hash_probes": {"value": null, "reason": "no hash dictionary in this build"}
  }
}
```

Missing metrics remain unavailable, not zero. Available values must carry the
exact definition from the config; if coverage differs, mark unavailable rather
than using the global counter name. Reset counters after setup and before the
first address; collect one input pass. The runner checks the address denominator,
normalizes counts per address/token, and compares release/diagnostic bytes to the
000-linked original and configured frozen references. A deliberate mismatching
file checks the byte comparison. Thread sampling is supplemental evidence;
too-short runs are explicitly incomplete. Source thread review is still required.

## Limits and pending validation

- No script execution, syntax check, builds or tests yet, per user instruction.
- No instrumented original/cpi/181 builds yet; legacy routed-event counts are
  not substituted for globally eliminated lookups.
- Stage/feature-family distributions, temporary live-byte tracking, detailed
  traversal/key-comparison counters and hardware profiles need separately
  defined instrumentation. They are not inferred by this runner.
- This script produces diagnostic JSON, not a final acceptance report. Fuzz,
  integration tests, counter calibration, ablations and uninstrumented
  speed/peak/steady memory benchmarks remain separate required validation.
- Retained process durations are command provenance, not speed measurements.

## Decision

Runner prepared for review; execution and architectural hypotheses unverified.
Ledger/activity updated. Professor performance results unchanged.
