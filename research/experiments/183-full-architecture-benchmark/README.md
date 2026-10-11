# Experiment 183 — full original, cpi, and integrated architecture benchmark

Date/time/timezone: 2026-10-06 America/Los_Angeles; pre-registered before suite run.
Status: **complete; release candidate rejected under the peak-memory constraint and does not beat cpi**. Original interrupted attempt preserved; AC timing resumed and completed.

## Hypothesis and change

Experiment 180 established an exact original-source route that carries state-word feature IDs from context into ordered scoring events. The release build in 181 may or may not be faster; no gain is assumed. Compare preserved original, the current cpi deliverable, and 181's release candidate in one suite session. Do not accept the new architecture without byte-identical output, one parsing thread, peak and steady memory no higher than original, and a measured improvement on the declared data.

No parser source is edited in this record. Baseline source is the upstream commit `25099c506612b34b23b1bfe286ca6321fcf06f35` archived in 176; baseline library is preserved at `research/experiments/000-baseline/artifacts/libpostal.a`. Candidate source and binary identities come from 181. Cpi source/artifacts remain unchanged.

## Reproduction

Use the suite's original baseline built against the preserved 000 library, plus cpi and 181 harnesses with identical inputs and model. Run all 13 registered datasets, including reserved v12/v13, explicitly consumed by the user's all-tests request. Before timing, pass integrity and correctness, including frozen references and two 2M-input fuzz seeds. Use serial, environment-gated, same-session timing with five rotating rounds and 20 passes. Record cold/warm results, per-dataset and per-country ratios, setup, peak and steady footprint, RSS, and `ps -M` thread counts. Save exact config, commands, hashes, timestamps, stdout/stderr/exits and all raw runs under the suite result path and this record. Stop or label invalid any failed gate or comparison; do not silently omit a dataset.

## All measured trials

**First attempt: no timed trial ran.** Timing started 2026-10-06 00:23:31 PDT and waited for the configured one-minute load to fall below 2.5. At 00:26:38 PDT the user said they needed to leave AC power soon; the assistant interrupted the wait with Ctrl-C (exit 130). The exact command, stdout/stderr, start/end, and interruption reason are in `raw/measurement.*`. At interruption, the suite run log had no `timing 1/195` entry, and no memory/thread/report stage had run. This initial attempt supplied no evidence of speed or memory.

The paragraph above describes the **first attempt only**. On the user's resume request, `run_measurement_resume1.sh` ran 2026-10-06 **11:39:59–12:23:36 PDT**, exit 0, under `caffeinate -i`. The AC gate passed; load settled below 2.5 at 11:40:59. All **195 trials** (13 datasets × 3 builds × 5 rounds) exited 0 and contained 20 passes, with rotating order and **zero gate failures**. The saved binary/input hashes were rechecked with zero mismatches before timing. Exact resume captures are `raw/measurement-resume1.*`; all trials, environment snapshots and report are in `research/suite/results/183-architecture-full-20261006/`.

Warm throughput, using the suite's source-weighted geometric mean of median paired ratios: **cpi 5.70× original; release candidate 1.13× original**. Across all 13 datasets without source weighting: **5.65× / 1.08×**. Direct same-round comparison puts cpi at **5.07× the candidate's throughput** when weighting sources equally. Five-round candidate intervals overlap 1.0 on v5, v6, v11 and v12; improvements are not demonstrated on every set. First-pass candidate ratios ranged 1.01–1.15× original. The full per-dataset, confidence-interval, setup and country results are in the suite's `report.md` and `results.json`.

| Build | Maximum peak footprint bytes | Steady footprint | Median setup seconds |
|---|---:|---:|---:|
| preserved original | 2,613,397,736 | 2492 MB | 2.77 |
| cpi | 2,543,405,432 | 1927 MB | 11.61 |
| release candidate | 2,615,118,056 | 2492 MB | 2.77 |

Candidate peak footprint exceeds original by **1,720,320 bytes** (about 1.64 MiB). Steady values match only at the tool's reported MB precision. Under the stated no-increase rule, this run does not qualify the candidate. Six live thread samples per build were all exactly one. Max RSS values are retained in the suite report alongside footprint.

## Correctness and interpretation

The 181 release build and 182 suite harness audit completed. The generated three-way config is `../182-benchmark-suite-prep/run-suite-3way-181.json` (SHA-256 `59c18cb15176954caab6babba3045b542231a96508b54ccab159683f571fef24`). Integrity and correctness ran serially 2026-10-06 00:15:53–00:23:00 PDT, exit 0, on all **13 registered datasets (128,540 addresses)** including v12/v13. The suite hashed all three variants and seven model files. All 26 candidate-versus-original dump comparisons matched byte-for-byte; cpi and release candidate each matched the original on both 2,000,000-input fuzz seeds. The four frozen golden-v1/novel comparisons returned `RESULT: IDENTICAL`, and the one-byte negative control detected a mismatch. Exact run logs, manifest, `correctness.json`, dumps and fuzz outputs are in `research/suite/results/183-architecture-full-20261006/`; stage command and captures are in `raw/correctness.*`. Reserved v12/v13 are now spent by this correctness run.

The suite runner reports dump/fuzz mismatches rather than failing its process on them, so `correctness.json` was inspected explicitly: `bad=[]`, every fuzz entry `identical=true`, frozen comparisons identical, negative control `detected=true`. Existing 180 oracles and the 181 smoke check are supplementary; this release-build suite correctness is the result of record.

## Decision

Correctness and live single-thread checks pass. **Reject the release candidate for acceptance:** maximum peak footprint exceeds original, and it remains about five times slower than cpi. The original-source architecture shows a modest whole-build improvement over original on the headline metric; that is not an attribution to the direct-ID mechanism alone because there is no matching-build unchanged-source control or ablation. The candidate uses O2 without PGO/LTO; cpi is the previously optimized deliverable with different build settings, documented in 181 and 119. This experiment does not disprove a fuller integrated architecture, but it does not establish one that beats cpi.

`analyze_contract.py` checked all trials, comparisons and thread counts and saved `raw/contract-audit.json`. Ledger and activity log updated. Professor report and deliverable remain unchanged because no new improvement was accepted. The complete requested benchmark report is now available; further experiments require their own hypotheses and records.
