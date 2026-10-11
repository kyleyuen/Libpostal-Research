# Experiment 201 final evidence audit

Audit timestamp: 2026-10-09T02:30:09-0700  
Scope: independent read-only check of the all-13 diagnostic aggregates, native API capture metadata, bounded golden-v1 timing/footprint/thread results, report corrections, and claims in `REPORT.md` / `README.md`. No build, parser, test, or benchmark was run during this audit.

## Findings

### Diagnostics and output gates

`raw/token-storage-diagnostics-v2/all13-diagnostic-v1/batch.json` records 26 completed captures: 13 matched CPI diagnostic controls and 13 199 diagnostic candidates. Each reports status pass, exit 0, empty stderr, and one parser thread. Both roles match the retained original output SHA in every corpus; the golden-v1 capture also passes the 12,863-row frozen ordered-component check with zero missing or mismatched rows.

Independent sum of per-corpus metrics confirms the report: 128,540 inputs and 688,817 tokens per role; matched CPI copies 688,817 retained tokens / 3,819,000 payload bytes. The candidate records 616,106 views / 3,531,981 referenced payload bytes and 72,711 fallback copies / 287,019 bytes. Copy + view counts, payload bytes, and NUL writes equal the matched CPI control exactly, per corpus and in aggregate. Opportunity and actual eligible/fallback address counters match. Normalized-text copy counters are identical. These are token payload copy/view counters, not heap allocation totals.

The same recomputation confirms equal generic instrumented counters in both diagnostic roles: 3,350,544 search API calls; 399,036 repeated queries; 1,930,029 materialized feature keys / 26,714,815 bytes; 5,530,942 model rows; 130,634,561 accumulated weights; and 385,767 tracked allocation calls / 30,031,553 requested bytes. Generic allocation instrumentation omits other translation units, so these figures do not establish library-wide allocation savings.

Source/build identity records under `artifacts/token-diagnostics-v4/{control,candidate}/hashes.json` point to the same CPI185 archive, source and switch manifests, and common diagnostic flags. The only mode delta is `-DEXP_TOKEN_SLICES`. The candidate is an instrumented diagnostic build, not a release timing binary.

### Native API memory and output

The base API capture plus repeated captures contain 10 unique command paths: round 1 has original 000, matched CPI+mimalloc, candidate+mimalloc, and CPI with the stock allocator; rounds 2 and 3 repeat the three main roles. All 10 have 78 serialized rows, the same stdout SHA-256 `02a8683c6611f80ee60cfc4d2ee5f2dff13eaf999bfe1a3938dab2ddc31a5fca`, byte identity true, and thread samples all one.

Across the repeated native API captures, maximum process peak footprint was 2,612,283,600 B for original 000, 2,517,469,560 B for matched CPI+mimalloc, and 2,517,485,968 B for 199+mimalloc. Candidate is 94,797,632 B below the original maximum and 16,408 B above matched CPI maximum. The original capture range is 136,675,424 B, versus a 16,408 B candidate range; these results are whole-process API measurements, not parser-only attribution.

The repeats manifest has a metadata defect: its round-2 and round-3 objects each appear three times but point to the same per-role command files. `analysis-recovered-v1.json` and raw paths identify the 10 unique captures; counting manifest array entries as captures would incorrectly report 22. The defect is preserved and explicitly accounted for here and in the final report.

### Uninstrumented golden-v1 screen

The corrected screen input has 12,863 rows, as shown by both configured golden-v1 text/TSV inputs and all 12 timing stdout records (`corpus=12863`). It ran three rounds of 200 passes for four roles; all 12 timing exits are zero. Recomputed pass counts are exactly 200 per capture and the summed warm time exceeds 10 seconds per trial. Thread captures report six samples per role, all one.

Warm medians are 32.208 µs/address for original 000, 5.286 for packaged CPI, 5.604 for source-matched CPI, and 5.558 for 199. The candidate is 0.8% below matched CPI by pooled median and 5.1% above packaged CPI. The source-attribution pair's per-round matched-CPI/candidate medians are 5.605/5.498, 5.560/5.5655, and 6.3095/5.5595. Thus the small pooled difference is round-sensitive and not a stable speedup claim. Later snapshots show elevated load and background activity; source-matched CPI round 3 records load 3.35 and AppleNeuralEngine/replayd activity. AC, Low Power Mode, and thermal/performance gate evidence passed; the one-minute load rose from 2.37 at settle to 3.90 in a later run snapshot.

The candidate maximum peak footprint is 2,537,359,736 B versus 2,612,709,584 B for original (75,349,848 B lower in this screen). Steady footprint was 1,927 MB versus 2,172 MB, rounded to whole MB. Rounded equality between candidate and CPI cannot prove the strict byte-precision steady-memory gate. The screen did not run correctness; exact-output claims come from the separate all-13 diagnostics, native API, and recorded fuzz captures.

The suite integrity stage was not selected, so `results.json` has `manifest: null`. The separate metadata-only record pins the four executables, config, input TSV, frozen reference, and model manifest without a parser invocation. Its row-count field mistakenly used 128,540 (the all-13 diagnostic total) for the 12,863-row single-dataset screen; `postrun-integrity-correction-v1.json` corrects the count while preserving the original record.

The suite's first final-report rendering failed on a missing dataset-address-count value after timing, memory, and thread data had been saved. A formatter-only source patch and offline recovery produced `raw/suite-report-fix-v1/report_recovered.md`; no parser rerun followed the patch. The unprotected first attempt and report-stage error remain preserved.

## Verdict and scope limits

The captured diagnostics establish large token-payload copy avoidance on these 13 observed datasets with exact accounting, while search/model/weight work and maximum tracked capacities remain unchanged. Output preservation and single-thread gates pass on the finite checked sets. The one-corpus timing screen does not support a stable incremental speedup over source-matched CPI; packaged CPI remains faster by its warm median. The screen's lower peak and rounded steady figures do not establish a full memory acceptance pass. The API maximum criterion is below original maximum but only 16,408 B above matched CPI, with substantial original variability.

No full multi-dataset timing/steady campaign, fresh unseen-set performance validation, or promotion was completed. Keep 201 as a bounded research result. All workload processes stopped after the screen; no follow-up experiment is queued.
