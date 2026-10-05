# Experiment 122 — re-test of word-bigram integer keys (EXP_INT_KEYS_PAIRS, 015 k2) on cpi

Date/time/timezone: created 2026-10-04T01:51:26-0700 (`raw/created.txt`); screen 2026-10-04T02:10:52-0700 → 2026-10-04T02:17:09-0700; America/Los_Angeles; commands in `raw/commands.txt`. Status: **not carried forward** (no speed mechanism; −48 MB peak noted)

Why: 117 counts about 9.6 bigram string keys per address. 015 found no gain from routing them through the integer table, but that was on a much older head. Change: switch only, `raw/switches_ipr.txt` = cpi + `-DEXP_INT_KEYS_PAIRS`.
Correctness: ipr is identical on golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2 (`raw/verify_ipr/summary.txt`).
**Dormant per-lookup verifier.** `EXP_INT_KEYS_VERIFY` aborts on plain rt8 too (`raw/rtv_dev.err`: `got=12`, a constant id).
- Cause: it has been incompatible with the stack since `EXP_CONST_IDS` (023), whose resolved pushes carry no verify string; 119's pushes have the same property.
- A repair in this tree (constant and 119 pushes now record their keys, `raw/address_parser.diff`) still aborts silently on the control (exit 134). Debugging under lldb with the 2 GB model was too slow and was stopped.
- Output identity was never affected: all accepted changes are checked end-to-end. This is a known gap in a debugging mode, recorded here.
Screen vs cpi (`raw/screen_analyze2.stdout`):
        v1 median cpi 5.196 ipr 5.194; ipr speedup per round [1.0019, 0.9985, 1.0, 1.0073, 1.0049, 0.9788] median 1.001; rounds faster 3/6
        v1 min    cpi 5.091 ipr 5.055; ipr speedup per round [1.0101, 0.9967, 1.0061, 1.0162, 0.9978, 0.9927] median 1.0019; rounds faster 3/6
        osm median cpi 3.357 ipr 3.294; ipr speedup per round [1.0295, 1.0267, 1.0067, 1.0174, 1.0312, 0.997] median 1.0221; rounds faster 5/6
        osm min    cpi 3.279 ipr 3.208; ipr speedup per round [1.0105, 1.0279, 1.0, 1.0255, 1.0259, 1.0052] median 1.018; rounds faster 5/6
Peak RSS: cpi 2,543,124,480 B, ipr 2,495,414,272 B (−48 MB). The bigram keys move from the string dictionary into the integer table.
Stage timers (`raw/split_analyze.stdout`):
    dev
       total_parse        cpi 5.539/2.441  ipr 5.630/2.469  Δcold +0.091 Δwarm +0.029
       context_fill       cpi 1.719/0.371  ipr 1.752/0.373  Δcold +0.034 Δwarm +0.002
       features           cpi 1.367/0.681  ipr 1.446/0.718  Δcold +0.079 Δwarm +0.037
       lookups            cpi 0.667/0.199  ipr 0.635/0.190  Δcold -0.032 Δwarm -0.009
       accumulate         cpi 0.804/0.467  ipr 0.812/0.464  Δcold +0.008 Δwarm -0.004
       viterbi_and_rest   cpi 0.470/0.424  ipr 0.470/0.424  Δcold +0.001 Δwarm +0.000
    osm
       total_parse        cpi 3.703/2.147  ipr 3.675/2.159  Δcold -0.027 Δwarm +0.011
       context_fill       cpi 0.982/0.340  ipr 0.980/0.340  Δcold -0.002 Δwarm +0.000
       features           cpi 0.872/0.562  ipr 0.887/0.588  Δcold +0.015 Δwarm +0.026
       lookups            cpi 0.412/0.164  ipr 0.377/0.153  Δcold -0.035 Δwarm -0.010
       accumulate         cpi 0.605/0.436  ipr 0.600/0.433  Δcold -0.005 Δwarm -0.004
       viterbi_and_rest   cpi 0.416/0.383  ipr 0.415/0.382  Δcold -0.001 Δwarm -0.001
Decision:
- **Mechanism.** Lookups gain about 0.03 µs, but feature generation loses 0.08 µs cold / 0.04 µs warm on dev, because both words of each pair must be canonicalized against the vocabulary. Net +0.09 µs on dev and −0.03 µs on OSM.
- **OSM screen.** Its +2.2% is not supported by the timers.
- **Outcome.** Not carried forward as a speed change. Its memory saving is noted in case a later change needs peak headroom.
