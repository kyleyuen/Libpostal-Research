# Experiment 053 — exact compression of the postcode-context graph (frees memory for speed structures)

Date/time/timezone: created 2026-10-02T18:03:53-0700; screens 2026-10-02T18:05:47-0700 → 2026-10-02T18:14:51-0700, America/Los_Angeles.
Status: **accepted as an enabler** (memory −323 MB; speed within ≈0.6% of `va`)

## Change (switch `EXP_PC_COMPRESS`, `sources/address_parser.c`, on `final/048-va` sources)
The postcode-context graph (5.48M rows, 122.3M sorted admin ids, 487.5 MB) is re-encoded at load as LEB128 gaps per row; a row is encoded only if non-decreasing (so graph_has_edge's binary search equals membership), every row is decoded and compared with the original before the original arrays are freed, and any failure keeps the original. `postal_code_context_exists` decodes the row and returns the same answer as `graph_has_edge`. Saving the model (`graph_write`) is not supported after compression (model saving was already guarded off in 025).

## Results
- Load check: `PC_COMPRESS rows=5483892 edges=122300179 bytes=151033002 (was 487.5 MB, now 165.0 MB)` (`raw/verbose.stderr`).
- Output byte-identical to the original on golden-v1, novel, OSM and 2,000,000 fuzzed inputs (seed 1) (`raw/verify_summary.txt`).
- Steady footprint: `va` 2,141 MB, **`pc` 1,818 MB**, `pc4` (load factor 1/4) 1,928 MB, `pc6` (1/6) 2,157 MB (`raw/steady.txt`, `raw/screen2/steady.txt`).
- Speed (sequential screens, golden-v1, 4 rounds each; `raw/screen_analyze.stdout`, `raw/screen2_analyze.stdout`):
    va [7.473, 7.448, 7.456, 7.48] median 7.465
    pc [7.516, 7.525, 7.511, 7.504] median 7.514
    pc4 [7.45, 7.511, 7.479, 7.48] median 7.479
    pc6 [7.467, 7.481, 7.461, 7.459] median 7.464
    pc vs va per round [0.9943, 0.9898, 0.9927, 0.9968]
    pc4 vs va per round [1.0031, 0.9916, 0.9969, 1.0]
    pc6 vs va per round [1.0008, 0.9956, 0.9993, 1.0028]
`pc` ≈0.6% slower than `va` (row decoding); sparser dictionaries recover that but add memory without a net gain.

## Decision
Keep `pc` as the base for memory-funded speed structures: steady headroom against the original's lower state (2,172 MB) grows from ≈31 MB to ≈354 MB.
