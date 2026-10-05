# Experiment 071 — feasibility of certified early exit (skipping feature lookups when the answer provably cannot change)

Date/time/timezone: created 2026-10-03T01:37:18-0700; runs 2026-10-03T01:38:04-0700 → ~01:00, America/Los_Angeles.
Status: complete — **not viable for this model** (measurement only)

## Idea
The parser returns only the label sequence (the Viterbi argmax). If, with a group G of features left out, the best path beats every other path by more than twice the most G could add along any path (plus a floating-point margin), the full computation must return the same labels, so G's lookups could be skipped without changing any output.

## Method (switch `EXP_CERT_MEASURE`, `sources/cert_measure.h` + hook in `crf.c`; -O2 build on 067 `pl` sources)
At first use: map every state/transition feature id to its name (dictionary string entries and integer-table keys) and compute model-derived bounds max |w| per (name, label) and per (name, transition cell). Per scored address and per group: name of every key from its bytes or integer key (without its lookup result); partial scores from the non-skipped features in the original order; bound arrays from the skipped features' names; Viterbi on the partial scores (y*); 2-best Viterbi on partial + bound; certified iff the best competing path's upper bound < y*'s lower bound − 1e-9 relative margin; certified answers checked against the full Viterbi path. Groups: word pairs; phrase-valued features; context (prev/next word + pairs); all transition features; all state features except bias and word roles; postcode features.

## Results (golden-v1 development, 10,305 scored addresses, 54.6 lookups/address; `raw/cert_run1.stderr`, `raw/cert_run2.stderr`)
    CERT addresses=10305 keys=562743 (54.6 per address)
    CERT group=0 [pairs] certified=0.0% group_keys=9.56/addr skippable_keys=0.00/addr (0.0% of all lookups) cert_but_wrong=0
    CERT group=1 [phrase-valued] certified=0.3% group_keys=16.20/addr skippable_keys=0.00/addr (0.0% of all lookups) cert_but_wrong=0
    CERT group=2 [context (prev/next word + pairs)] certified=0.0% group_keys=18.64/addr skippable_keys=0.00/addr (0.0% of all lookups) cert_but_wrong=0
    CERT group=3 [all transition features] certified=0.0% group_keys=8.42/addr skippable_keys=0.00/addr (0.0% of all lookups) cert_but_wrong=0
    CERT group=4 [all state features except bias/word roles] certified=0.0% group_keys=28.62/addr skippable_keys=0.00/addr (0.0% of all lookups) cert_but_wrong=0
    CERT group=5 [postcode] certified=3.5% group_keys=2.48/addr skippable_keys=0.00/addr (0.0% of all lookups) cert_but_wrong=0
    MARGIN full-score best minus second-best path: p10=8.613 p50=30.215 p90=47.248
    BOUND group=0 2x path bound of skipped features: p10=567.826 p50=946.688 p90=1513.415
    BOUND group=1 2x path bound of skipped features: p10=293.890 p50=901.241 p90=1774.156
    BOUND group=2 2x path bound of skipped features: p10=1079.932 p50=1822.350 p90=2968.200
    BOUND group=3 2x path bound of skipped features: p10=415.704 p50=708.441 p90=1195.539
    BOUND group=4 2x path bound of skipped features: p10=1233.406 p50=2019.291 p90=3475.888
    BOUND group=5 2x path bound of skipped features: p10=56.073 p50=292.431 p90=292.431

Output of the instrumented build identical to earlier development dumps; no certified address had a different answer (`cert_but_wrong=0`).

## Conclusion
Decision margins (median 30) are 20–60× smaller than any sound bound built from per-name maximum weights (median ≈700–2,000), because some features of every name carry very large weights; almost no address certifies, so 0.0% of lookups could be skipped. Tighter per-key bounds would require reading the per-key data the method aims to skip, and the 2-best decode would cost about as much as the saved lookups. Certified early exit is not a viable speedup for this model under exact output. (Research idea 6.3 in ALGORITHM_RESEARCH.md is hereby measured.)
