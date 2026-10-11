# Experiment 203 publication documentation audit

Date: 2026-10-10 (America/Los_Angeles). Read-only claim/link/table review except for the current handoff pointer and new dated handoff note.

## Checks

- Checked `README.md`, `research/suite/README.md`, `research/experiments/README.md`, the 203 experiment README/report, promoted 203 README, `research/HANDOFF.md`, the dated 2026-10-07 handoff, and `research/ACTIVITY_LOG.md`.
- Parsed Markdown tables with Python `markdown-it-py` and checked relative Markdown links with a small Python path-existence check.
- Commands used: `rg -n '203|202' ...`; a Python `MarkdownIt('commonmark').enable('table')` table/link check; and `sed`/`tail` inspections of ledger, handoff, activity log, and reports.

## Results

- Relative links resolve in the reviewed documents. Parsed table row counts, including headers: root README 7; suite README 10; experiment ledger 205 (header plus IDs 000–203); 203 final report tables 4, 15, and 4.
- Ledger row 203 reports 14 datasets / 130,341 rows, 42 paired timing/memory captures, cohort-specific warm ratios (6.11× golden, 5.83× novel, 5.55× OSM, 5.72× v14), negative peak-footprint and max-RSS deltas, and limited default-parser acceptance. These match the 203 final report and run paths.
- Root README separates 203 claims from experiment 202’s older packaged-119 results and Senzing regression. The 203 report and promoted README state that v14’s cached original came from 202, original fuzz was not rerun, and full public-API parity was not validated.
- Suite README accurately says v12–v14 were already observed in 183/202 and describes setup-only. `benchmark/run.sh` defaults to source 203 and allows `--cpi-source 119`. No `benchmark/README.md` exists; setup instructions are in root and suite READMEs.
- Activity log has a dated 203 promotion entry and preserves the historical 119/202 entries. The existing top-level handoff pointer still described pre-203 state, so it now points to a new 2026-10-10 handoff; the 2026-10-07 historical handoff remains unchanged.

No measured-result correction was needed. Experiment 202 and earlier dated reports remain preserved as historical records.
