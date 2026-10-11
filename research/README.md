# Research directory index

Start with the [root README](../README.md) and [REVIEW_GUIDE.md](../REVIEW_GUIDE.md). This page explains what else is in `research/`.

## Required for reviewing the 203 result

| Path | Contents |
| --- | --- |
| [METHOD.md](METHOD.md) | Measurement protocol, gates, and acceptance rules. |
| [final/203-cpi-memory/](final/203-cpi-memory/README.md) | Promoted candidate: source, build record, PGO profile, tested binaries, hashes. |
| [experiments/README.md](experiments/README.md) | Ledger of all 204 experiments (000–203), including rejected and inconclusive ones. |
| [experiments/203-cpi-memory-regression/](experiments/203-cpi-memory-regression/README.md) | The experiment that produced the current candidate, with its final validation report. |
| [suite/](suite/) | Validation runner, its configuration, and raw results (`suite/results/203-stream-*`). |
| [tools/](tools/) | Harnesses and verifiers, including `verify_203_clone_evidence.py`. |
| [audits/](audits/) | Independent audits of the 203 outputs, timing, and memory evidence. |
| [analysis/203-startup-breakeven/](analysis/203-startup-breakeven/README.md) | Startup cost and break-even computed from the 203 raw captures. |

## Supporting history

| Path | Contents |
| --- | --- |
| [final/](final/) | Build records of earlier accepted candidates (021, 048, 062, 077, 119, and others). |
| [analysis/](analysis/) | Cross-experiment analyses and limit studies. |
| [archive/](archive/) | Superseded reports and organization snapshots, kept unedited. |
| [ACTIVITY_LOG.md](ACTIVITY_LOG.md) | Chronological log of research sessions (timestamps in UTC). |
| [package/](package/) | Release and clone-review manifests used to check the published package. |
| `EXCLUDED_FILES.sha256`, `ORACLES_v1_SHA256SUMS.txt` | Sizes and hashes of large or binary files that are kept out of git. |

## Working notes (not required reading)

These were written during the work and are kept unedited because experiment records link to them. Some statements in them were later superseded; the README, METHOD.md, and the 203 records are authoritative.

| Path | Contents |
| --- | --- |
| `HANDOFF.md`, `HANDOFF_2026-10-*.md` | Session-to-session handoff notes. |
| `MORNING_SUMMARY_2026-10-04.md` | Status summary at that date. |
| `OUTCOME_REPORT_2026-10-09.md` | Report on the earlier 202/119 build. Its pooled "source-weighted" speed figure is not used for 203, whose cohorts are deliberately not pooled. |
| `plans/` | Plans written before experiments ran; plans are not results. |
| `newwrapupnotes/` | Drafts and fact maps used while writing the final documents. |
