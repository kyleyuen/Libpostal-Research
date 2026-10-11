# Final-registry standalone counter calibration

This calibration used the final `diag.c` registry, including the corrected cpi `EXP_PS_FLAGS` definitions. It is separate from the earlier `calibration-20261007` attempt and the parser smoke in `smoke-20261007-1520-linkfix`.

## Compile and assertion commands

At `2026-10-07T21:48:53Z` the harness was compiled with exit 0:

`/Users/kyle/Library/Application Support/Headroom/headroom/bin/rtk clang -std=gnu2x -Wall -Wextra -DEXP185_DIAG -I research/experiments/185-diagnostic-instrumentation/source/original research/experiments/185-diagnostic-instrumentation/source/diag.c research/experiments/185-diagnostic-instrumentation/tools/calibrate_diag.c -o research/experiments/185-diagnostic-instrumentation/artifacts/calibration-20261007-final-registry/calibrate_diag > research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007-final-registry/compile.stdout 2> research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007-final-registry/compile.stderr`

Compiler stderr is in `compile.stderr` (319 bytes; warning: `diag_write_metrics` does not use its caller-provided `tokens` argument and instead reports its instrumented token counter). The three executable runs and their stdout/stderr captures are `basic`, `query-overflow`, and `pattern-overflow`; each exited 0. Their JSON output mtimes are 2026-10-07 14:49:03–14:49:04 PDT. The validator exited 0 at 14:49:11 PDT:

`/Users/kyle/Library/Application Support/Headroom/headroom/bin/rtk python3 research/experiments/185-diagnostic-instrumentation/tools/validate_diag_calibration.py research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007-final-registry/basic.json research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007-final-registry/query-overflow.json research/experiments/185-diagnostic-instrumentation/raw/calibration-20261007-final-registry/pattern-overflow.json`

Validation output is `validation.stdout`: `diagnostic calibration assertions passed (basic, query cap, pattern cap)`. Cases assert exact hit/miss/repeat counts, pointer-distinct dictionary identities, pattern aggregation behavior, same-address reuse, reset between addresses, complete by-stage sums, and truthful query/pattern cap metadata.

## Results

- **Basic:** 5 search calls, 4 hits, 1 miss, 2 executed repeats separated by dictionary pointer, 1 exact same-address no-search reuse, and no repeat across address reset. By-stage calls sum to 5. Stable export merges the same-family rows into one pattern (`address_groups=2`, `queries=5`, `executed=4`, `reused=1`).
- **Query cap:** 4,098 calls; two calls beyond the 4,096 per-address identity cap are reported as untracked. The exact repeated count and pattern totals are null, with one overflow address and a lower-bound repeated count of zero.
- **Pattern cap:** 129 exact repeated groups and 258 observations; 128 groups retained and one omitted with two omitted observations. Coverage is explicitly first-encountered, not top-by-frequency.

SHA-256: final `diag.c` `c5bbc3d91a8ba21d49b5c925d4e6af360007ba2f7a26dd8cd310ee7ce219ed5e`; harness binary `deea392312b00db661264f1a45bb9c57016a1e56f785066e5e52fc49c05ce704`; harness source `2a755b3520fbdc15076ed7501743822b896fef1bc8caa06c0060ea0aa37a8b82`; validator `745736a2238e700332b64f964f6d8688edfa0eabc7931bb1087b3bb13df4dbb5`; basic JSON `81a46811c10cb8651abcac942d00b243a956379ff7e9788e30a8ec9991b00e9b`; query-cap JSON `32e4c94bb63ebd9dbf2425e14779c42da3f368adce426796ef88a328a6bd78f1`; pattern-cap JSON `88b9fb21ff73679099960645106ff61ce8c82ef4953d342d15c19fd5ea5603d2`.
