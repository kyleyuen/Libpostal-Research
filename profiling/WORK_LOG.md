# Libpostal performance work log

## 2026-09-28 — Initial inspection and profile

Objective: evaluate a possible 3.5x single-threaded parsing speedup over two months
without increased memory use. No speedup has yet been implemented or demonstrated.

Inspected feature generation, trie lookup, sparse CRF scoring, Viterbi, context
allocation/reset, parser setup and the build. Existing `src/bench.c` benchmarks
expansion of a repeated input, not parsing. Created a separate C parser harness.
The knowledge base had no libpostal-specific project history; verification-first
delivery and explicit uncertainty informed the workflow. Applied ponytail guidance.

Installed Homebrew autoconf 2.73, automake 1.19 and libtool 2.6.2 (m4 dependency);
Homebrew auto-updated during installation. Built the existing checkout with debug
symbols and upstream optimization levels, native arm64, no tracked library edits.
Downloaded the checkout-selected v1.0.0 base and parser archives from
`https://github.com/openvenues/libpostal/releases/download/v1.0.0/` to
`/Users/kyle/.cache/libpostal-profile/`, extracted under its `libpostal/` directory.
Archives and extracted data remain available locally. No language classifier loaded.

Initial extraction yielded 70 unique test literals, but an embedded newline split
one input into two lines, producing 71 harness inputs in an exploratory run.
Corrected extraction by replacing embedded newlines with spaces and reran sampling
and all reported timing trials on 70 inputs. The superseded exploratory sample
was overwritten before archival; no results from it are used in the final report.

Three clean 10-second runs: 45,436.38 / 46,142.62 / 45,852.89 addresses/s;
median 45,852.89, peak RSS up to 2,611,396,608 bytes. Sampled a separate 30-second
run for 15 seconds: 12,860 main-thread samples. Trie lookups 28.0%, feature
generation 19.1%, normalization 13.2%, Viterbi 4.0%, scanner 0.9% (inclusive,
overlapping shares). Scanner is built at -O0, but not a major measured bottleneck.

Interpretation: prioritize feature representation/lookup and CRF scoring.
Potential repeated clearing/marginal-buffer work warrants a future experiment.
No proof of 3.5x; improving the entire measured scoring stage alone has an
approximate 2.87x infinite-speed ceiling on this particular small corpus.

## 2026-09-28 — Archive and golden corpus

User requested persistent results/logging and a larger golden address dataset.
Copied the original harness, binary, raw sample, results and report to `initial-70/`.
Copied build/configuration/download logs to `setup-logs/`. Existing original files
were retained. This log records performed work, not a reconstructed full terminal
transcript; installation stdout and the superseded exploratory run are summarized.

Downloaded Senzing's public v1.2.0 test CSV, README and license at pinned commit
`5113929832ac06619d18a4816f78a90fef8cc7a3`. Chosen because it is an existing
multicountry libpostal evaluation corpus with component annotations and public
provenance, avoiding synthetic repetition or downloading huge training archives.

Built golden-v1 with all 12,868 source rows, 12,863 unique timing inputs, 88 countries,
5,399 non-ASCII inputs. Preserved all annotations. Hash-based country-stratified
development/validation split: 10,308 / 2,555. No content normalization.

The initial timing harness had a 10,000-input cap; replaced its fixed pointer array
with a dynamically grown array. This modifies only the external profiling harness.
Recorded exact default-model outputs for all 12,863 addresses with a ctypes checker;
reran and obtained zero mismatches. These outputs are a compatibility reference,
separate from supplied annotations. They do not assert parser correctness.

Golden timing/profile results, hashes, source URLs and environment are saved with
this log. No library optimization, model retraining, Git commit or publication
was performed. Generated libpostal build files remain for reproducibility.

Golden-v1 timing trials: 27,542.83 / 28,602.47 / 27,821.28 addresses/s;
median 27,821.28. Peak RSS up to 2,613,886,976 bytes. This is the baseline for
future v1 comparisons. The full-corpus sampling run attributes 48.7% to trie
lookup, 15.3% to feature generation, and 2.3% to Viterbi (inclusive shares).
The larger workload strengthens the case for investigating feature lookup.

Verification: deterministic dataset rebuild was byte-identical; exact baseline
output check passed on 12,863 inputs; deliberately altered reference produced
exactly one mismatch and exit status 1. Model archives, extracted model files,
libraries and profiling artifacts were hashed. The dataset was not used to train
or modify any model. Source training overlap remains unknown.
