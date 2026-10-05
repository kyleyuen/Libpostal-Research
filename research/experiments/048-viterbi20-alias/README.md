# Experiment 048 — Viterbi specialized for 20 labels; transition matrix aliased instead of copied

Date/time/timezone: created 2026-10-02T17:32:20-0700; builds and output checks ~18:40, screen 2026-10-02T17:33:53-0700 to 2026-10-02T17:37:17-0700, America/Los_Angeles.
Status: screen positive; `va` carried forward (full validation pending with later changes)

## Changes (experiment copy of `l3` sources; `raw/base_sources.sha256`)
- `EXP_VITERBI20` (`crf_context.c`): for L = 20, the existing NEON Viterbi written with ten named register pairs instead of arrays (same operations, same order per lane: score = (prev[i] + state_trans[t][i][j]) + trans[i][j], i = 0..19, first strictly greater kept). 047 measured the recursion at 0.70 µs/address.
- `EXP_TRANS_ALIAS` (`crf.c`): the context's transition matrix points at the model's instead of being copied (3.2 KB) per address; restored before the context is destroyed; the other scoring paths copy only if not aliased (016 T8's C3 without C2). A first edit also re-enabled the C2 bookkeeping block; reverted before building.

## Checks
Builds `v20`, `al`, `va` (`l3` PGO/LTO/mimalloc procedure) all exit 0; golden-v1 dumps byte-identical to the original (sha 45574ee2…, `raw/*_v1.jsonl`).

## Screen (`tools/screen.sh`, sequential, rotating order, golden-v1, 20 passes, 4 rounds; `raw/screen_analyze.stdout`)
    l3 [7.744, 7.77, 7.755, 7.763] median 7.759
    v20 [7.478, 7.46, 7.434, 7.678] median 7.469
    al [7.733, 7.718, 7.703, 8.347] median 7.726
    va [7.416, 7.367, 7.407, 7.472] median 7.412
    v20 vs l3 per round [1.0356, 1.0416, 1.0432, 1.0111]
    al vs l3 per round [1.0014, 1.0067, 1.0068, 0.93]
    va vs l3 per round [1.0442, 1.0547, 1.047, 1.0389]

`va` is faster than `l3` in every round (≈+3.9 to +5.5%, median 7.41 vs 7.76 µs). The alias alone is within noise.
