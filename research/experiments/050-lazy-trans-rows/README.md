# Experiment 050 — lazy per-row initialization of transition scores (on 048 `va`)

Date/time/timezone: created 2026-10-02T17:39:49-0700; screen 2026-10-02T17:41:42-0700 to 2026-10-02T17:44:15-0700, America/Los_Angeles.
Status: **rejected (slower than `va`)**

## Change
Switch `EXP_ST_LAZY` on 048's `va` sources: no per-address clearing of the T×L×L transition-score matrix; a per-token bitmask marks rows (previous label i) that a transition feature wrote, zeroing each row on its first write; the L = 20 Viterbi uses prev + trans for unmarked rows (exactly (prev + 0.0) + trans, since alpha is never −0.0); other L materialize zeros first. Basis: 049 warm profile (memset 5.7% of warm samples, all from the per-address clearing).

## Checks
Build `lz` (switches in `raw/switches.txt`): output byte-identical to the original on golden-v1, novel and OSM, both frozen references identical, 2 × 2,000,000 fuzz identical (`raw/verify_summary.txt`).

## Screen (sequential, rotating, golden-v1, 20 passes, 4 rounds; `raw/screen_analyze.stdout`)
    l3 [7.845, 7.817, 7.818, 8.464] median 7.832
    va [7.461, 7.45, 7.473, 7.762] median 7.467
    lz [7.809, 7.848, 8.084, 7.928] median 7.888
    va vs l3 per round [1.0515, 1.0493, 1.0462, 1.0904]
    lz vs l3 per round [1.0046, 0.996, 0.9671, 1.0676]
    lz vs va per round [0.9554, 0.9493, 0.9244, 0.9791]

`lz` is 2–8% slower than `va` in every round: the per-entry row check and division, per-row clears and the branch in Viterbi cost more than the removed clearing. Rejected; `va` reconfirmed (+4.6–5.2% vs `l3` in rounds 1–3).
