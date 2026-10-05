# Experiment 090 — split of "Viterbi and rest" for pcp (diagnostic)

Status: complete. 047's timers ported to pcp sources (`sources/crf.c`: score setup = crf_tagger_score entry to the direct path; Viterbi = crf_context_viterbi), `EXP_STAGE_STATS -DEXP_VT_STATS`, PGO/LTO/mimalloc; harness `tools/bench_split_vt.c` (043 harness printing ss_t[13..14]); development and OSM, 2 runs (`raw/split_*`).
Development cold/warm µs: score setup 0.196/0.164, Viterbi recursion 0.388/0.387 (pure compute), Viterbi-and-rest total 0.662/0.618. OSM: 0.169/0.143, 0.349/0.348, 0.588/0.556. The recursion is ≈13% of warm time on OSM → 091.
