# Experiment 040 — is feature generation instruction-bound or miss-bound? (diagnostic)

Date/time/timezone: created 2026-10-02T16:54:58-0700; measurements 2026-10-02T16:55:46-0700 to 2026-10-02T16:58:50-0700, America/Los_Angeles.
Status: complete (measurement only)

## Method
Build `dupf` = `l3` + diagnostic switch `EXP_DUP_FEATURES` (`sources/crf.c`): for every scored address, all tokens' features are generated once and discarded before the real pass, so the real pass runs with warm caches. `l3` PGO/LTO/mimalloc procedure. Output byte-identical to the original on golden-v1 (sha 45574ee2…, `raw/dupf_v1.jsonl`). Instructions and cycles per address vs `l3` by the 2-vs-102-pass difference method, 3 rotating reps, sequential, AC-gated (`tools/run.sh`, all exits 0).

## Results (`raw/analyze.stdout`)
    l3 instructions retired [72553, 72554, 72566] median 72554
    l3 cycles elapsed [30753, 30890, 31837] median 30890
    dupf instructions retired [89529, 88839, 89543] median 89529
    dupf cycles elapsed [36323, 34592, 34934] median 34934
    extra (warm) feature generation: +16975 instructions, +4043 cycles, IPC 4.20

A warm repeat of feature generation costs ≈17,000 instructions (23% of `l3`'s ≈72,550 per address) and ≈4,040 cycles (≈1.0 µs at ≈4 GHz, IPC 4.2). In place (038) the stage takes ≈2.15 µs, so roughly half of feature generation is instruction work and half first-touch memory cost.

## Interpretation
A leaner feature generator (no string joins, fewer strlen/memmove, no neighbour recomputation) could remove part of the ≈1.0 µs instruction share (estimated 0.5–1.0 µs, 6–12%, UNVERIFIED); the ≈1.1 µs first-touch share needs denser data layout, which the memory margin limits (039).
