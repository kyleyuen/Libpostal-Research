# Public report fact map (draft)

| Public claim | Current stale text | Replacement source / rule |
|---|---|---|
| Goal | Root README names a person and adds the performance target. | State the user's current goal plainly: faster parsing on one thread, exact tested outputs, and no increase in peak or steady memory. |
| Main speed, spread, memory | Root README headline uses the 2026-10-04 run. | Use only the final 202 report and its same-run baseline/candidate results. Keep the old run historical in its dated suite report. |
| Correctness coverage | Root README says 11 sets and older fuzz totals; it also discusses unseen v12/v13. | Use 202's run manifest and explicitly say all thirteen sets have already been observed. Say “tested” with exact corpus/fuzz/API scope; never “all inputs.” |
| Build | Root README's list may omit current source details and claims a ceiling. | Verify archive/source hashes, flags, PGO profile input, LTO, scanner flags, mimalloc object, model hash and architecture from 119/202 build records. Name NEON as one-thread SIMD only. |
| Memory | Old root README uses prior rounded totals. | Copy exact peak and steady values from same-run 202 report. Do not treat RSS or Python/C API harness footprint as suite acceptance. Include setup/load time only if 202 reports it. |
| 201 token-span research | Candidate metrics are easy to overstate as a CPI win. | State it is not promoted; include matched-control comparison, counter coverage, fallback cost, bounded timing/memory limitations, and native API caveat from 201 report plus final audit. |
| Queue status | Parallel-exploration README said no workloads had started. | Updated to closed 188–200 queue, with 201 complete and 202 in progress; update again when the 202 report is final. |
| Reserved sets | Suite README calls v12/v13 “never-used unseen” and says the flag spends them. | Correct to “historically reserved”; experiment 183 already observed all thirteen. |
| Historical ceiling | Root README has a current “ceiling without changing rules” section, though later caveats supersede it. | Remove from the current presentation. Preserve the archived analysis and label it as a retired, bounded projection rather than a global proof. |
| Links and table formatting | Existing four inspected READMEs had no broken relative Markdown links. Root summary table is much longer than needed. | Keep the new main README to 8–12 compact bullets and link the detailed report; verify all new links and display tables before finish. |
