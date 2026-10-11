# Experiment 165 — diagnostic design study: direct feature generation/shared token IDs

Date/time/timezone: 2026-10-05 22:37 PDT (workspace environment labels current date 2026-10-06; shell clock captured in `raw/evidence.txt`).
Status: inconclusive — bounded O3i probe completed; no candidate built/timed.

## Hypothesis and change

Opportunity: exp 157 attributed roughly 1.0 µs of cpi's 2.1 µs cold feature+lookup work to string building and lookup; exp 160 tried model-compiled records and direct (record,slot) emission but was slower than cpi (0.71–0.83×). Exps 141–149 also show that computing and probing integer IDs per token/pair costs as much as or more than current string emission. A different architectural hypothesis is to generate each feature family directly into a single ordered stream of pre-resolved feature IDs while context fill establishes token identity once, sharing IDs already computed for words and phrase matches. Avoid per-feature-class dispatch, memo checks, and repeated vocabulary/canonical probes through compile-time specialized emitters or a compact token descriptor.

Risk: 160's direct record path and 149's integer path already test close variants; those results strongly lower prior odds. The only distinct potential is fusing token identity production with context fill so the avoided work exceeds the added bookkeeping. Exact feature-ID order and duplicates must be retained because score accumulation order is observable. Direct generation must also preserve print_features API behavior and all feature-family fallbacks.

Falsifiable next probe: use O3i stage oracles to count/categorize repeated token identity derivations and build a work-count model for a fused descriptor path. Stop if it adds any per-token probe/branch beyond current cpi or saves less work than exp 149's measured extra cost. Only then prototype one feature family behind a switch and compare ordered O3i streams before timing.

## Bounded O3i probe

Ran `python3 research/experiments/165-direct-feature-emission/probe.py golden-v1 osm-v3 fuzz-s1-200k` (exit 0; stdout/stderr/exit in `raw/probe.*`). The script streamed resolved-feature oracle records; golden-v1 and OSM were complete, while fuzz was capped at the first 500,000 feature records (48,329 token positions) to bound memory. Input and script hashes: `raw/input_script_hashes.txt`.

| Corpus | Feature records | Distinct resolved IDs | IDs reused at other token positions | IDs spanning addresses | Duplicate same-token ID records |
|---|---:|---:|---:|---:|---:|
| golden-v1 | 702,370 | 133,821 | 45,758 | 36,951 | 5,480 |
| osm-v3 | 546,890 | 29,585 | 19,294 | 18,499 | 3,388 |
| fuzz-s1-200k (first 500k records) | 500,000 | 87,247 | 38,238 | 29,597 | 3,453 |

Unresolved `-` IDs: 98,994 / 51,128 / 81,681. Examples in stdout show resolved IDs at token positions. Repeated IDs cannot be deduplicated because each ordered feature event contributes to a score; this count alone does not show shared token descriptors save work.

## Reproduction and provenance

Hardware/OS: Apple Silicon arm64, Darwin 25.5.0 (`raw/evidence.txt`). Compiler/build: none; no candidate or benchmark. Base commit: `181737d30977adc9d287e3f7a1cfafd7277525d4`. cpi source SHA-256: `address_parser.c` `a3db12f9d2daf32903f986197a1738eef5d383a5528d94e8e8100b0804f18536`; `crf.c` `cc001bb7f932e3c435e59290e309b415f3c468ed53caf74eb95b4506c8048860`. Command output in `raw/evidence.txt`.

## Trials and correctness

No candidate, model, binary, test, or timing measurement was run. This probe read frozen O3i oracles only; therefore there is no speed or memory result. Byte-identical output on all golden-v1 inputs and single-thread parsing are hard constraints. No existing artifact was overwritten.

## Decision

Inconclusive, with substantial negative prior from 149 and 160. This probe finds ID reuse but cannot identify avoided work. Next action is a bounded work-count comparison against cpi memo/canonicalization behavior using existing counters; kill the idea if the saving is not distinct from 149/160. Ledger/report/activity log intentionally untouched per task scope.
