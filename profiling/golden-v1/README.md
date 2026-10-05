# Golden benchmark v1

Frozen on September 28, 2026 for single-threaded libpostal runtime optimization
with no increase in memory and exact preservation of existing parser outputs.

## Inputs

- 12,863 unique address strings from 12,868 original records, 88 country codes.
- 5,399 strings contain non-ASCII characters; maximum UTF-8 length is 191 bytes.
- `all.txt`: complete benchmark, deterministic SHA-256 order.
- `development.txt`: 10,308 inputs for profiling and optimization experiments.
- `validation.txt`: 2,555 inputs reserved for milestone/final checks.
- `records.jsonl`: input IDs, split, countries and every original annotated row.
- `raw/`: unchanged downloaded CSV, upstream README and license.
- `baseline-outputs.jsonl`: exact ordered `(label, component)` outputs from the
  current default model. These preserve behavior; they are **not ground truth**.

Source: [Senzing/libpostal-data v1.2.0 public test data](https://github.com/Senzing/libpostal-data/blob/5113929832ac06619d18a4816f78a90fef8cc7a3/files/tests/v1.2.0/test_data.csv),
pinned commit `5113929832ac06619d18a4816f78a90fef8cc7a3`.
The published subset omits records the publisher could not release. The downloaded
subset's counts above, rather than the full private set's counts, define v1.

Raw source labels: 12,692 OSM, 140 libpostal, 36 Libpostal-bug records. Preserve
the supplied Apache-2.0 repository license and source attribution. OSM-derived
data attribution: © OpenStreetMap contributors, https://www.openstreetmap.org/copyright.
The repository license should not be assumed to erase underlying data obligations.

## Selection and freeze rules

All public v1.2.0 records are retained. Only exact duplicate input strings are
collapsed in timing files (five records); original annotations remain in
`records.jsonl` and the raw CSV. No case folding, Unicode normalization, address
repair, or synthetic expansion was applied. Empty strings, NULs and line breaks
are rejected by the builder; none occurred in this source.

Input ID = SHA-256 of exact UTF-8 address bytes. Within each country, sort by ID
and reserve the first `max(1, floor(n/5))` records for validation. Remaining
records are development. If an input has multiple country codes, its first
sorted code determines split grouping; all codes remain recorded. Final file
order is ID order. Development and validation are disjoint by exact input string,
but may include different strings describing the same location.

Use development for day-to-day decisions. The complete corpus was benchmarked
once during establishment of v1; avoid repeatedly tuning against validation.
Validation is a held-aside optimization workload, **not a demonstrated unseen
ML test set**: overlap with model training data is unknown. This is also a
country-skewed test corpus, not a production traffic distribution. It includes
short and difficult inputs but does not establish long-input/adversarial coverage.
Future larger or production-specific corpora should be separate versions.

Do not edit v1 inputs, labels or baseline outputs in place. Hashes are in
`../SHA256SUMS`; provenance and model hashes are in `../manifest.json`.
Supplied annotation errors or disagreements with libpostal should be logged
separately, not silently corrected. No annotation-accuracy percentage is claimed.

## Run and check

From the workspace root:

```sh
profiling/parser_profile profiling/golden-v1/development.txt 10
python3 profiling/check_outputs.py check
```

For a candidate library, use `check_outputs.py check --library /absolute/path/to/libpostal.dylib`.
The checker compares every ordered label/value pair and exits nonzero on a mismatch.
It is not a timing tool. `record` refuses to overwrite existing baseline outputs.
For fair speed comparisons, compile the C timing harness against the candidate
library with the same compiler/flags and model files as the baseline.

Time only parsing and response destruction after setup and a full warm-up pass;
one parsing thread, no sampler during timing, at least three runs, report median
throughput and peak RSS. Record hardware, compiler, model hashes, input hash and
binary hash. Keep every trial, including regressions. Peak RSS includes the model,
input corpus and startup; compare with the same corpus and harness. Also examine
steady-state memory for future changes; v1 currently records peak RSS only.

Building more addresses into RAM changes harness memory relative to the initial
70-input experiment. Compare candidates to the v1 baseline, not the old 70-input
RSS value. A 3.5x claim must preserve output checks and memory constraints on the
declared workload; the frozen corpus alone cannot prove all-input equivalence.
