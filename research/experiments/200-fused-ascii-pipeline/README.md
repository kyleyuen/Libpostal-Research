# Experiment 200 — fused ASCII normalization and tokenization

Date/time/timezone: started 2026-10-07 19:24 PDT; v2 release built 2026-10-07 19:47 PDT, America/Los_Angeles.
Status: corrected v9 development/golden diagnostic screen completed and candidate stopped before all13 because golden eligibility was 24/12,863 (0.19%).
Evidence: source-backed narrow path, scanner helper checks, release build, and corrected dev/golden output/thread checks; no speed or memory result.

## Hypothesis

For an ASCII-letter/digit/space address, the configured normalization is byte-length-preserving lowercase, and the unchanged scanner can emit token spans from those same source bytes. A one-pass wrapper around the scanner can lower/copy each scanner-consumed byte while recording its exact token type and span, eliminating the separate normalization traversal and tokenization traversal on this narrow input set. Any unsupported byte or unexpected scanner behavior abandons the temporary result and uses the original normalizer plus `tokenize_add_tokens`; the candidate never mutates caller input or changes scanner/token feature rules.

Implementation starts from experiment 119, not 199. It does not depend on token-span storage, experiment 198's certifier, or a cache. The source guard is based only on current input bytes and scanner behavior.

## Implementation history and helper checks

V1 was built and handed off briefly, then withdrawn before screening when the model-free helper showed that `scan_token()` advances its cursor one byte past the input terminator. Its `scanner.cursor == scanner.end` check therefore forced fallback for all inputs. V1 source, build, and original/corrected descriptors remain at `sources-v1/`, `artifacts/release-v1/`, and `raw/handoff-v1*.json`; the failed helper output is in `raw/scanner-test-v1/`.

V2 uses emitted token spans to prove that scanner-consumed bytes are contiguous from offset zero and cover the full input. It avoids relying on the scanner's post-token cursor position. The model-free harness extracts the exact helper from the candidate source and checks normalized bytes and token type/span records against `tokenize(lower(input))`. The check passed every string of length 1–6 over `AaZz09 `, targeted scanner-sensitive examples (`I`/`IV`/`XII`, ordinal suffixes, `1E2`, `St`/`PO`/`UK`/`NW`, and mixed address phrases), a 4096-byte stress string, and fallback cleanup for empty, tab, punctuation, and UTF-8 inputs. This checks the helper's span/type equivalence only; it is not a parser/model golden test or a universal equivalence proof. Source, harness and results are in `raw/scanner-helper-test.c`, `raw/scanner-test-v2.*`, and `raw/handoff-v2.json`.

The first attempt to compile the expanded helper together with `scanner.c` at `-O2` was interrupted after roughly three minutes of single-core compilation. The exact command and exit 130 are recorded in `raw/scanner-helper-compile-o2-interrupted.md`; terminal output was not redirected and cannot be represented as complete archived compiler logs. The helper was subsequently linked to experiment 198's unchanged scanner object built at `-O0` (`06cfaf25be695731afa6e0b63a9f773a7d2a6e52c7315325767aeef6a442a381`) and the test passed. This model-free check did not invoke the parser or model.

## Decision

V2 build exit 0 and model-free helper checks pass. Corrected v9 diagnostics matched retained original outputs on development and golden-v1 inputs with one observed thread, but only 24 of 12,863 golden-v1 addresses (0.19%) took the fused path. All 12,839 other golden-v1 inputs fell back after added scan/copy work; aggregate allocation was higher than the CPI control by 12,839 calls and 514,363 requested bytes. It was stopped before all13; speed and memory were not measured. A bounded future direction and the equivalence requirements are recorded in `REPORT.md`.
