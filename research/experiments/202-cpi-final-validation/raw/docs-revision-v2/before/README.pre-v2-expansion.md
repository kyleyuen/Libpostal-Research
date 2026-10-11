# Libpostal address parser optimization

- **Goal:** Faster default parsing on one thread while preserving ordered labels, components, checked outputs, and original memory limits.
- **How:** CPI reduces repeated searches and text work with model-based tables, an address-label decoder, compiler tuning, and vector instructions within one thread.
- **Timing:** Warm parsing was about **5.9× faster** across 14 datasets. CPI startup was slower; the detailed report explains both averaging methods.
- **Memory:** The full-suite peak was **70.5 MB below** original. The Senzing pair still exceeded its subgroup peak limit by **59.0 MB**; OSM and v14 subgroup peaks were lower.
- **Correctness and limits:** Outputs matched byte-for-byte on 14 sets (130,341 rows); sampled golden-v1, OSM v3, and v14 runs observed one thread. The public feature-print API has a known CPI abort, so this does not establish full API parity. OSM load varied substantially; see the evidence report.
- **Details:** [Evidence and experiment history](research/OUTCOME_REPORT_2026-10-09.md), [measurement rules](research/METHOD.md), [experiment ledger](research/experiments/README.md), [202 records](research/experiments/202-cpi-final-validation/).
