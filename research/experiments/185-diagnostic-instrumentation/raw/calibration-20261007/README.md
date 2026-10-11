# Diagnostic runtime calibration

This standalone calibration was performed after the initial six builds completed and before the `filter_checks` / `searches_avoided` definition-only correction. The latter changed schema text, not counting or JSON serialization logic; a fresh harness run is still required against the final rebuilt runtime.

## Commands and results

- Compile at `2026-10-07T21:45:57Z`:
  `/Users/kyle/Library/Application Support/Headroom/headroom/bin/rtk clang -std=gnu2x -Wall -Wextra -DEXP185_DIAG -I research/experiments/185-diagnostic-instrumentation/source/original research/experiments/185-diagnostic-instrumentation/source/diag.c research/experiments/185-diagnostic-instrumentation/tools/calibrate_diag.c -o research/experiments/185-diagnostic-instrumentation/artifacts/calibration-20261007/calibrate_diag`
  Exit 0. Compiler warning: unused `tokens` parameter in `diag_write_metrics`.
- `calibrate_diag basic raw/calibration-20261007/basic.json`: exit 0. Asserted 5 actual lookup calls, 4 hits, 1 miss, 2 exact repeated searches (one per dictionary), 1 same-address no-search dedup, and zero cross-address repeats. By-stage lookup calls reconciled to 5. The stable family pattern row aggregated the two same-family dictionary instances (`address_groups=2`, `queries=5`, `executed=4`, `reused=1`), documenting the known aggregation caveat.
- `calibrate_diag query-overflow raw/calibration-20261007/query-overflow.json`: exit 0. Asserted 4,098 calls, 2 untracked calls after the 4,096 identity cap, one overflow address, null exact repeated/pattern totals, and lower-bound repeated count 0.
- `calibrate_diag pattern-overflow raw/calibration-20261007/pattern-overflow.json`: exit 0. Asserted 129 exact repeated groups, 128 retained, 1 omitted, 258 observations, 2 omitted observations, and `coverage=first_128_encountered_repeat_groups`.
- `/Users/kyle/Library/Application Support/Headroom/headroom/bin/rtk python3 research/experiments/185-diagnostic-instrumentation/tools/validate_diag_calibration.py research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007/basic.json research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007/query-overflow.json research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007/pattern-overflow.json`: exit 0, printed `diagnostic calibration assertions passed (basic, query cap, pattern cap)`.

## Artifacts

The command's stdout/stderr captures are the matching `.stdout` and `.stderr` files beside each JSON output. JSON outputs are the raw calibration records. The final validator output is recorded above; a re-run will be saved after the final six-build set.

SHA-256:

- Harness binary `calibrate_diag`: `deea392312b00db661264f1a45bb9c57016a1e56f785066e5e52fc49c05ce704`
- Harness source: `2a755b3520fbdc15076ed7501743822b896fef1bc8caa06c0060ea0aa37a8b82`
- Validator source: `745736a2238e700332b64f964f6d8688edfa0eabc7931bb1087b3bb13df4dbb5`
- Basic JSON: `81a46811c10cb8651abcac942d00b243a956379ff7e9788e30a8ec9991b00e9b`
- Query-overflow JSON: `32e4c94bb63ebd9dbf2425e14779c42da3f368adce426796ef88a328a6bd78f1`
- Pattern-overflow JSON: `88b9fb21ff73679099960645106ff61ce8c82ef4953d342d15c19fd5ea5603d2`
