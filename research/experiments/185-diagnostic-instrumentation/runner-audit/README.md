# Experiment 185 runner audit

Status: runner hardening and frozen-reference comparison repair are tested; stable label getters and their nested/overflow calibration passed; final six-build config is ready for corrected smoke, then parent-authorized full run.
Evidence type: diagnostic tooling only. No libpostal process or model workload was run.
Workspace base commit: `181737d30977adc9d287e3f7a1cfafd7277525d4` (runner files were untracked at audit start).

## Scope and changes

Audited `research/tools/run_measurements.py` and
`research/experiments/184-operation-measurements/test_runner.py` against
`research/METHOD.md`, the approved measurement plan, experiment 184, and the
185 coverage checklist. The project knowledge base had no Libpostal/project
memory card; implementation choices follow direct repository evidence.

The runner now:

- preserves scalar counter definitions and stage/family/repeated-pattern
  metadata, rejecting unknown metric and dimension names, invalid values,
  incomplete repeated-pattern summaries, and counter arithmetic mismatches;
- keeps missing stage/family cells explicitly unavailable, never zero, and
  verifies configured complete stage partitions against whole-pipeline totals;
- enforces the 13-dataset serial run when `--run` is used, checks the AC gate
  before each child process, and retains the gate output;
- compares baseline output to frozen references and compares each release and
  diagnostic output byte-for-byte to the original, with a deliberate negative
  comparison control;
- records exact argv, wrapper argv, environment overrides, timestamps, exit
  codes, stdout/stderr, thread samples, SHA-256 inputs/builds/model/provenance,
  and per-process `/usr/bin/time -l` output;
- parses macOS time output for peak RSS and peak memory footprint, and reports
  process durations only as provenance/diagnostic overhead evidence, never as
  product speed;
- samples the dumper PID inside the time wrapper's isolated process group,
  rejects multiple dumper processes or any sampled thread count other than one,
  and terminates the whole process group if interrupted.

An early unlogged unittest attempt found a `NameError` in the new per-address
normalizer (`expected_addresses` was not passed into its helper). The attempted
output is retained in `runs/attempt-unlogged-000.txt`; its timestamp and exit
code were not captured, so neither is inferred. The fix passes the denominator
explicitly. Subsequent test runs are fully captured as JSON records.

## Verification

Commands ran from the workspace root through Headroom RTK:

| Command | Time (America/Los_Angeles) | Result | Evidence |
|---|---|---|---|
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/probe_time.py` | 2026-10-07 14:12:13 | exit 0; verified `/usr/bin/time -l -o`, parsed 15,056,896-byte max RSS and 6,881,688-byte peak footprint from raw output | `runs/time-probe.json`, `runs/time-probe.txt` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/probe_capture.py` | 2026-10-07 14:19:14–15 | exit 0; one dumper child and five valid `ps -M` samples, each reporting one thread | `runs/capture-probe-serial.json`, `runs/capture-probe-serial/` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_test.py` | 2026-10-07 14:19:39 | exit 0; 7 unit tests passed | `runs/attempt-006.json` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_test.py` | 2026-10-07 14:42:26 | exit 0; 8 unit tests passed, including overflow summary unknowns | `runs/attempt-007.json` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_test.py` | 2026-10-07 14:48:55 | exit 1; new required repeated-pattern metadata exposed an incomplete test fixture; fixture corrected, no runtime behavior change | `runs/attempt-008.json` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_test.py` | 2026-10-07 14:49:05 | exit 0; 8 unit tests passed with corrected pattern fixture | `runs/attempt-009.json` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/investigate_frozen.py` | 2026-10-07 | exit 0; trusted existing comparator reports 12,863 rows, zero missing and zero ordered-component mismatches | `../raw/frozen-reference-investigation/comparison.json` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_test.py` | 2026-10-07 | exit 0; 9 unit tests pass, including differing JSONL shapes with identical semantic rows, altered-component negative control, and component-order mismatch | `runs/attempt-011.json` |
| `bash research/experiments/185-diagnostic-instrumentation/tools/run_diag_calibration.sh 20261007-label-intern4` | 2026-10-07 15:15 onward | exit 0; basic/cap/nested getter cases and explicit overlength/pool-full rejection cases passed | `../raw/calibration-20261007-label-intern4/` |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_test.py` | 2026-10-07 | exit 0; 9 unit tests passed after semantic frozen check change | `runs/attempt-012.json` |
| `bash research/experiments/185-diagnostic-instrumentation/tools/build_variants.sh --build` | 2026-10-07 | exit 0; final six sibling set `20261007-1540-stable-labels` compiled with stable runtime labels and diagnostic dumper define | `../raw/build-20261007-1540-stable-labels/` |
| `python3 research/tools/run_measurements.py --config research/experiments/185-diagnostic-instrumentation/config.json` | 2026-10-07 | exit 0; preparation-only check found no missing inputs/artifacts, all 13 datasets, no processes launched | active config points to final build |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_final_preflight.py` | 2026-10-07 | prep exit 0 with no missing files; subsequent AC gate exit 1 because an active `dump_outputs` process was detected; no workload launched by this capture | `runs/final-preflight-and-gate.json` |
| `bash research/experiments/185-diagnostic-instrumentation/tools/build_variants.sh --preflight` | 2026-10-07 14:42 onward | exit 0; required source, tools, model directory, and preserved baselines present | build session records |
| `bash research/experiments/185-diagnostic-instrumentation/tools/build_variants.sh --build` | 2026-10-07 14:40–15:10 | two failed attempts preserved; final build ID `20261007-1505-final-registry` compiled all six release/diagnostic siblings serially with exit 0 and frozen-source hashes | `../raw/build-20261007-1440-retry1/`, `../raw/build-20261007-1450-rebuild/`, `../raw/build-20261007-1455-delivery-flags/`, `../raw/build-20261007-1505-final-registry/` |
| `python3 research/experiments/185-diagnostic-instrumentation/tools/generate_config.py --build-id 20261007-1505-final-registry` | 2026-10-07 | exit 0; config generated from runtime `diag-schema.json` | `../config.json` |
| `python3 research/tools/run_measurements.py --config research/experiments/185-diagnostic-instrumentation/config.json` | 2026-10-07 | exit 0; preparation-only preflight, all 13 datasets and required artifacts available, no processes launched | command output retained in task transcript |
| `python3 research/experiments/185-diagnostic-instrumentation/runner-audit/capture_test.py` | 2026-10-07 | exit 0; 8 unit tests passed after latest runner/schema edits | `runs/attempt-009.json` |

Unit coverage includes exact file comparison and mismatch detection, invalid or
overflow counts, boolean/empty denominators, scalar coverage and normalization,
stage/family normalization and total reconciliation, repeat-pattern arithmetic
and truncation metadata (including overflow-unknown totals and schema-required
semantic keys), unknown coverage keys, `ps -M` PID-column parsing,
`/usr/bin/time -l` RSS/footprint parsing, and retention of failed-command logs.

The AC gate passed in the parent task before the final build; the parent owns
the run gate for all later workloads. The harmless probes
above did not load libpostal, read the model, or run any of the 13 datasets.
They are tooling checks, not counter calibration, output validation, memory
acceptance, or speed measurements.

## Build attempts and current full-run command

- The first build attempt exposed an empty-array failure under macOS Bash `set -u`; it stopped before compilation and is retained in `raw/build-20261007-144018-89524/`.
- The next two builds exposed missing production `EXTRA_SW` switches. Those failed compiler logs remain in `raw/build-20261007-1440-retry1/` and `raw/build-20261007-1450-rebuild/`; their binaries are not used by the final config.
- The corrected build uses all 21 base CPI switches plus every flag in `research/final/119-cpi/switches.txt`, including `EXP_FA_LEN`, `EXP_PS_FLAGS`, and `EXP_CP_IDS`. The final 185 CPI source is a diagnostic-instrumented mirror of the 119 source; the final build uses the shipped switch set and does not use compatibility fallbacks.
- Final config binds all three variants' release and diagnostic dumpers/libraries to one six-build source snapshot, and the runner manifest hashes all build artifacts plus raw build command/provenance files.
- The first final-source edge smoke found that diagnostic libraries were instrumented but the dumpers were linked without `-DEXP185_DIAG`; exit-zero smoke results lacked metric sidecars and are explicitly invalid for instrumentation. The build link command is fixed. Relink-only ID `20261007-1520-diagdumper-linkfix` was an intermediate correction set and is not the active config.
- The first `full13-20261007` runner attempt stopped at golden-v1 before candidate execution because it byte-compared two different JSONL representations. Frozen rows store `id` plus ordered components; preserved dumper rows store address plus ordered components. The old attempt is preserved at `results/full13-20261007/` and has zero candidate records. Applying the repository's existing `015-integer-keys/tools/compare_frozen.py` to the captured original gives 12,863 rows, zero missing, zero ordered-component mismatches, `IDENTICAL`; raw evidence and first-row shapes are in `raw/frozen-reference-investigation/comparison.json`. The runner now uses the same address-hash keying and exact ordered component comparison; it saves the comparator summary and verifies a deliberate altered-component negative control. Frozen reference files are untouched.
- Instrumentation coverage found a runtime stage/family/operation metadata pointer restoration bug. `diag_current_*` now returns process-lifetime interned strings; setters intern their inputs, and address reset leaves snapshots stable. Labels over 63 bytes or beyond 512 distinct interned values now produce an explicit stderr error and cause `diag_write_metrics` to return -2, preventing silent category merging. The nested-save calibration verifies stage, family, and operation restoration after nested setters and address reset. The overlength and full-pool cases verify explicit failure.
- Coverage reviewer approved the stable-pointer implementation and calibration. The final matching six-build ID is `20261007-1540-stable-labels`; active config is `185/config.json`. An AC gate passed after build/config, but a later gate capture detected an active `dump_outputs` process; no workload was launched by this runner task. Corrected real smoke is parent-coordinated. The first full-corpus attempt remains preserved with zero candidate records.
- Repeated-pattern rows aggregate semantic family, operation, and exact key bytes across addresses; actual per-address repeat equality remains dictionary-pointer exact. Distinct same-family dictionary objects can collapse into one exported row, so use these distributions as semantic-family summaries.
- Parent-coordinated calibration and tiny edge-address checks are currently pending. Do not launch the 13-corpus run until parent authorizes it after smoke review. The command/output location reserved for that run is:

  `python3 research/tools/run_measurements.py --config research/experiments/185-diagnostic-instrumentation/config.json --run --output research/experiments/185-diagnostic-instrumentation/results/full13-retry1`

  Full diagnostic data, release-versus-diagnostic overhead interpretation, output equality, and parser thread/memory evidence remain pending. No speed or memory improvement is claimed here.
