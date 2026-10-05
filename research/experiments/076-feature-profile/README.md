# Experiment 076 — feature-generation section profile of the head (diagnostic)

Status: complete (diagnostic). 074 sources + 038's feature-section timers ported (`sources/address_parser.c`, `EXP_FS_STATS`, 8 marks) + `EXP_STAGE_STATS`; 043 `bench_split.c` with -DHAVE_FS; development.tsv, 3 runs (`raw/split_fs_r*.stdout`).
Cold/warm µs per address (run 2): setup 0.060/0.053, address dict 0.080/0.054, component 0.262/0.114, **postcode 0.375/0.102**, word 0.209/0.149, trans word 0.081/0.058, prev word 0.208/0.132, **next word 0.452/0.212**.
Postcode-context counters (`EXP_PC_STATS` build `pcs`, `raw/pcs.stderr`): development 14,761 calls (1.43/address), 64 bytes decoded per call, mean row 177 bytes; OSM 16,299 calls. → 077 (prefetch rows) and 078 (eager vocabulary memo, aimed at the next-word section).
