# Experiment 042 — warm cost of lookups + weight accumulation; weight entries per address (diagnostic)

Date/time/timezone: 2026-10-02, ~17:20–17:35 PDT (`raw/run.log`, `raw/created.txt`).
Status: complete (measurement only)

## Methods
- `dupl` = `l3` + diagnostic switch `EXP_DUP_LOOKUP` (`raw/crf_c_dupl_version.c`): the two dictionary lookups and a full accumulation into scratch rows are run once before the real ones, so the real pass is warm. `l3` PGO/LTO/mimalloc; output byte-identical to the original on golden-v1 (sha 45574ee2…). Instructions/cycles vs `l3`, 2 vs 102 passes, 3 reps (`tools/run.sh`, `raw/analyze.stdout`).
- `nnz` = `l3` + `EXP_NNZ_COUNT` (-O2, counts only): weight entries accumulated per address. First attempt's patch did not apply (two identical loop headers in `crf.c`); noted in `raw/run.log` and redone on the correct loop.

## Results
- Warm lookups + accumulation: **+20,394 instructions, +6,935 cycles (≈1.7 µs, IPC 2.94)** per address vs in-place lookups 1.16 + accumulation 1.08 µs (038) → this stage is mostly instruction work; only ≈0.5 µs is first-touch cost.
- Per scored address (`raw/nnz_summary.txt`, development split): 45.9 state features (38.6 found), **478 state weight entries** (12.4 per row); 8.7 transition features (8.3 found), **615 transition weight entries** (73.8 per row) — ≈1,093 scattered additions per address.

## Interpretation
Lookups+accumulation cost is dominated by executing ≈1,100 scalar scattered adds plus string-key hashing, not by misses. State rows are on average 62% dense over the 20 labels; transition rows 18% of 400 cells.

## Correction (added after experiment 043)
043 measured each stage's cold (first call) and warm (immediate repeat) time inside the same process: lookups 1.21 → 0.49 µs and accumulation 1.12 → 0.51 µs, i.e. ≈1.33 µs of the ≈2.33 µs is first-touch cost. The `dupl` estimate above (≈1.7 µs warm) included the cost of the extra scratch accumulation and lookup pass itself, so the statement "mostly instruction work" was wrong; 043's split supersedes it.
