# Experiment 203 — CPI peak-footprint regression fix

Date: 2026-10-10 (America/Los_Angeles). Status: validated and promoted for the measured default-parser scope. The promoted candidate is [`research/final/203-cpi-memory`](../../final/203-cpi-memory/README.md). It does not replace historical experiment 119 or the preserved original 000 comparator.

## Result

The streamed postcode-graph loader avoids retaining the full 122,300,179-edge index array while allocating the compressed representation. It reuses the existing query decoder and falls back to the prior graph reader on unsupported or malformed input. The tested v2 candidate logged an active stream marker for all 42 candidate timing captures, all 14 output dumps and both fuzz checks.

All 14 candidate output dumps matched cached original outputs byte-for-byte (130,341 rows). Two 2-million-input fuzz digest/status checks matched historical original references. All 84 paired timing captures exited 0 and exceeded 10 seconds of parsing-only work. All 42 paired peak-footprint and max-RSS comparisons decreased; all three representative steady-footprint pairs decreased. Six thread-capture processes, across three corpora and two binaries, produced six live samples each, all at one OS thread. Thread captures were intentionally terminated after sampling and are separate from exit-0 timing runs.

Timing was collected on one shared Mac. The candidate's setup medians were higher than the original's; warm parsing ratios and per-cohort limitations are in [`FINAL_VALIDATION_REPORT.md`](FINAL_VALIDATION_REPORT.md). Speed ratios are not pooled across cohorts. Unseen-v14's cached original output came from a fresh original-baseline run in experiment 202, not a new original dump from the 203 run.

## Evidence paths

- Full results, per-dataset peak footprint/RSS delta ranges, steady and thread samples, build identity and limitations: [`FINAL_VALIDATION_REPORT.md`](FINAL_VALIDATION_REPORT.md).
- Candidate source and accepted binaries: [`research/final/203-cpi-memory`](../../final/203-cpi-memory/README.md).
- Byte-output, negative-control and digest/status check: `research/suite/results/203-stream-output-v1/`.
- Warm timing and paired peak captures: `research/suite/results/203-stream-gold-novel-v1/`, `research/suite/results/203-stream-osm-v3-v13-v1/`, `research/suite/results/203-stream-v14-v1/`.
- Steady and live-thread evidence: `research/suite/results/203-stream-steady-golden-v1/`, `research/suite/results/203-stream-steady-osm-v3-v1/`, `research/suite/results/203-stream-steady-v14-v1/`, and `raw/stream-steady-thread-v1/thread-captures/`.
- CP_IDS ablation, failed first streaming capture and earlier build failures remain preserved in the versioned `raw/` and suite result directories; the fallback capture is excluded from validation.

## Acceptance limits

The pass applies to the default-parser behavior and recorded datasets. It does not establish full public-API parity or portability across operating systems. The prior 78-call API evidence belongs to experiment 119 and was not repeated against the 203 build. The known save-after-load limitation under `EXP_PC_COMPRESS` predates 203 and remains. Model files, PGO profile and mimalloc object are external inputs whose hashes are recorded; they are not bundled in the promoted source tree.
