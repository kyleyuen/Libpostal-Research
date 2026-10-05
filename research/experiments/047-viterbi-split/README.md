# Experiment 047 — split of "Viterbi and rest" (diagnostic)

Date/time/timezone: 2026-10-02 ~18:30 PDT (`raw/run.log`).
Status: complete (measurement only)

Stats build `vt` = `l3` + `EXP_STAGE_STATS` + `EXP_VT_STATS` (timers around the scoring setup in `crf_tagger_score` before the direct path, around `crf_context_viterbi`, and around the label-string loop in `crf_tagger_predict`); `l3` PGO/LTO/mimalloc procedure; `tools/stage_bench.c` prints the three extra values. Two 50-pass runs on the golden-v1 development split (`raw/vt_r1.stdout`, `raw/vt_r2.stdout`): score setup 0.190 / 0.190 µs, Viterbi recursion 0.700 / 0.698 µs, label strings 0.057 / 0.057 µs (stage "Viterbi and rest" 1.014 / 1.013 µs). Basis for 048.
