# Experiment 201 — CPI token-span work and memory

Date: 2026-10-09  
Status: bounded measurements complete; no promotion claim  
Protocol: `research/METHOD.md`

Experiment 201 paired CPI and 199 token-span diagnostics, then ran correctness and a bounded uninstrumented golden-v1 screen. The diagnostics show 616,106 token views / 3,531,981 referenced payload bytes instead of copies on eligible spans across the 13 observed datasets; fallback covers the remaining inputs. Search, model-row and scoring-weight counters did not change. Maximum tracked container capacities also did not fall.

The 3-round × 200-pass screen covered 12,863 golden-v1 rows and completed timing, steady-footprint and thread stages for original 000, packaged CPI, source-matched CPI, and 199. Suite thread samples were all one. This screen did not run its correctness stage; byte-output checks are from the separate all-13 diagnostics and native API captures. The suite’s final Markdown formatter failed on a missing address-count field after measurements were saved. The original row-count metadata error is preserved with a correction addendum; see `REPORT.md` and the raw evidence.

The observed warm median was 5.558 µs/address for 199, 5.604 for source-matched CPI, and 5.286 for packaged CPI. 199 peak footprint was 2,537,359,736 B, 16,384 B above matched CPI and below the original’s 2,612,709,584 B. Steady measurements are rounded to MB and cannot establish a precise gate. Native API peak differences are small between candidate and matched CPI and remain host/setup-level measurements. The run experienced elevated background load in later rounds, so the three-round timing is exploratory. These are bounded screening results, not a stable speedup or blanket memory-pass claim.

See [REPORT.md](REPORT.md) for full counters, artifacts, failures, limits, and conclusion. The independent final evidence review is in [AUDIT-FINAL-v1.md](AUDIT-FINAL-v1.md); the earlier diagnostics-only audit is in [AUDIT-DIAGNOSTICS-v1.md](AUDIT-DIAGNOSTICS-v1.md). Prepared work and source-level reasoning remain in [CPI_WORK_PLAN.md](CPI_WORK_PLAN.md) and [MEMORY_CHECK_PLAN.md](MEMORY_CHECK_PLAN.md).
