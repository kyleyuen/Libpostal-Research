# Experiment 202 — accepted CPI final validation

**Status: complete for the declared 14-dataset set.** The unchanged accepted
experiment 119 CPI matched original 000 output byte-for-byte on 130,341 inputs,
passed the single-thread check, and had a lower global maximum peak footprint.
The Senzing subgroup still had a +58,982,640 B peak-footprint difference, and
CPI startup was slower (median setup 11.70–11.72 s vs 2.76–2.90 s original).

The source-weighted geometric mean of the 14 per-dataset median paired-round
ratios is **5.893289×**; the unweighted geometric mean is **5.806217×**. These
are warm parse ratios with setup excluded. See [the final validation report](FINAL_VALIDATION_REPORT.md)
for per-dataset results, settings, load conditions, memory/thread scope,
correctness evidence, and limitations.

## Runs and evidence

- Senzing golden-v1 + novel: `202-final-senzing-v2`, 3 rounds × 200 passes.
- OSM v3 + unseen-v4–v13: `202-ac-osm-v3-v13-resumed-v1`, 3 × 420.
- Fresh unseen OSM v14: `202-ac-unseen-v14-resumed-v1`, 3 × 2300.
- Raw suite results and manifests: `research/suite/results/<run-id>/`.
- The prior partial AC OSM run, battery preparations, and stopped attempts are
  preserved under `raw/` and `research/suite/results/`; they are not included
  in the completed AC timing summary.

The 13 previously observed output comparisons used bytewise CPI JSONL vs cached
original outputs. V14 used fresh original and CPI dumps with complete-stream
byte comparison. Fuzz was two 2M seed digest/status comparisons, not exact
serialized-output fuzz. Prior experiment 201 has 78 safe adapted-library API
cases, but that is not a fresh 202 feature-print/API test. No parser source
change or build was made in this validation.

## Evidence map

- [Measurement protocol](../../METHOD.md): timing, output, thread, and memory gates.
- [Detailed results](FINAL_VALIDATION_REPORT.md): dataset ratios, cohort settings,
  environment, memory/thread scope, correctness, and limitations.
- [Machine-readable provenance](raw/final-validation-provenance-v1.json) and
  [original final-document hashes](raw/final-document-manifest-v1.sha256).
- [Fresh v14 dataset input](../../../profiling/golden-v14-osm-unseen/unseen14.txt) and
  [tabular benchmark input](../../../profiling/golden-v14-osm-unseen/unseen14.tsv).
- [Senzing cohort](../../suite/results/202-final-senzing-v2/) and
  [OSM cohorts](../../suite/results/202-ac-osm-v3-v13-resumed-v1/) plus
  [v14 cohort](../../suite/results/202-ac-unseen-v14-resumed-v1/).
- The v1 document bytes are preserved under
  [the v2 revision snapshot](raw/docs-revision-v2/before/); current document
  hashes are recorded in [the v2 document manifest](raw/docs-revision-v2/current.sha256).
