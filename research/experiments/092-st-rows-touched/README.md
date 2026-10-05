# Experiment 092 — how much of the state-transition score matrix is touched per token (measurement)

Date/time/timezone: created 2026-10-03T10:24:43-0700, America/Los_Angeles. Status: complete — **idea not viable**

Idea: skip clearing (score setup ≈0.16 µs, 090) and the Viterbi addition for state-transition rows that no transition feature touches. Counter `EXP_STROW_STATS` on 091 vmx sources (-O2; `raw/zs_*.stderr`):
    development: STROW_STATS addresses=10000 tokens(t>=1)=48767 touched source rows/token=14.82 of 20, touched cells/token=104.7 of 400, tokens with none=10.6%
    OSM:         STROW_STATS addresses=11000 tokens(t>=1)=47624 touched source rows/token=15.13 of 20, touched cells/token=110.5 of 400, tokens with none=10.8%
About 15 of 20 source-label rows (≈105 of 400 cells) are touched per token and only ≈11% of tokens touch none, so row-level skipping saves little; cell-level clearing (≈105 scattered stores) is no cheaper than the contiguous memset (016 T8 and 066 found the same). Not pursued.
