# Experiment 204 — public-API parity of the 203 candidate

Date/time/timezone: 2026-10-10 23:22–23:44 PDT (2026-10-11 06:22–06:44 UTC)
Status: completed. Result: 7 of 8 API families identical; `libpostal_parser_print_features(true)` aborts in 203 (known, not fixed)
Evidence type: correctness diagnostic (no timing claims)
Protocol: METHOD.md; correctness only, one process at a time, one thread each

## Hypothesis and change

Question: earlier records stated that public-API parity of the CPI candidate beyond the default parser was unverified, after a feature-print probe aborted in 119. Does 203 return byte-identical results to the original for every exported libpostal API family?

No parser source was changed. New test tooling:

- `research/tools/api_suite.c` (SHA-256 `8fb99815…3a3d` at attempt 2) is a harness that calls every exported function family in `libpostal.h`, in eight modes:
  - `parse_options`: `libpostal_parse_address` with six language/country option sets.
  - `expand`: `libpostal_expand_address` and `_root`.
  - `classify`: `libpostal_classify_language`.
  - `dedupe`: near-dupe hashes (plain, with languages, name hashes), `place_languages`, the seven `is_*_duplicate` pair checks against the previous address, and the fuzzy name/street duplicate checks.
  - `toponym`: `libpostal_is_toponym_duplicate`.
  - `normalize`: tokenize, `normalize_string` (plain and with languages), `normalized_tokens` (plain and with languages).
  - `print_features`: `libpostal_parser_print_features(true)`, 20 parses, then off and 20 more.
  - `lifecycle`: three setup/teardown cycles parsing the same 200 addresses.

  Each mode is a separate process with line-buffered stdout.
- `research/tools/api_parity.py` (SHA-256 `b460123…c8a0`) runs each mode on both builds sequentially and compares stdout, stderr and exit status byte-for-byte. Stderr is also compared with `(file.c:LINE)` numbers masked; in this run raw stderr was identical anyway.
- `benchmark/run.sh --api` / `--api-only` build the harness for both versions and run the comparison. The language classifier model (`language_classifier.tar.gz`, 50,334,317 bytes, v1.0.0 release) is needed only here and is now downloaded on demand. The hash of `language_classifier/language_classifier.dat` is recorded in `benchmark/language_classifier_sha256.txt` (`f0800eb0…a277`), taken at first download on 2026-10-10.

## Reproduction

- Hardware/OS/compiler: Apple M4 Pro (Mac16,8), macOS 26.5.2, Apple clang 17.0.0 (clang-1700.6.3.2), on AC power.
- Builds: fresh `benchmark/run.sh` builds in a scratch `WORK_DIR`, from pinned libpostal `25099c50` and the promoted 203 sources, with PGO retrained by setup. These are not the archived 203 artifacts. The harness binary hashes are in each attempt's `api/api_parity.json`; for attempt 2 they are original `635670ae…`, optimized `347f7900…`.
- Input: `profiling/golden-v1/validation.txt` followed by `profiling/golden-v14-osm-unseen/unseen14.txt`, 4,356 addresses, SHA-256 `ff070362a7b967f2…`; the full hash is in `api_parity.json`.
- Command: `WORK_DIR=<scratch> ./benchmark/run.sh --api-only`. The log of each attempt is `raw/attempt-N-*/run.sh.log`.

## All attempts

| Attempt | Harness difference | Result | Raw |
|---|---|---|---|
| 1 (23:30:30–23:32:26 PDT) | `is_toponym_duplicate` was called inside `dedupe` | `dedupe` crashed identically in both builds (SIGSEGV, exit −11) after 190 of 4,356 lines, so most dedupe calls went untested. `print_features` failed. The other 5 modes were identical. | [raw/attempt-1-20261010-233030/](raw/attempt-1-20261010-233030/) |
| 2 (23:41:56–23:44:14 PDT) | toponym moved to its own mode | 7/8 identical; `print_features` failed | [raw/attempt-2-20261010-234156/](raw/attempt-2-20261010-234156/) (includes the exact harness and driver copies) |

Before attempt 1, a harness bug was found and fixed in a 5-address smoke test (not preserved). The harness released the `place_languages` array with `libpostal_expansion_array_destroy`, which crashed. Upstream returns the classifier's own array, whose strings are not owned (`language_classifier_response_destroy` frees only the array), so the harness now calls `free()`. The exact attempt-1 harness was not saved separately; it differed from the attempt-2 copy only in where the toponym call was placed.

## Results (attempt 2)

| Mode | Original exit / lines | Optimized exit / lines | Identical |
|---|---|---|---|
| parse_options | 0 / 26,136 | 0 / 26,136 | yes |
| expand | 0 / 8,712 | 0 / 8,712 | yes |
| classify | 0 / 4,356 | 0 / 4,356 | yes |
| dedupe | 0 / 4,356 | 0 / 4,356 | yes |
| toponym | −11 / 189 | −11 / 189 | yes: both crash identically |
| normalize | 0 / 21,780 | 0 / 21,780 | yes |
| print_features | 0 / 159 | −6 (abort) / 1 | **no** |
| lifecycle | 0 / 600 | 0 / 600 | yes |

## Interpretation and limits

- **`print_features` is a real API difference.** With feature printing switched on, 203 stops with `EXP_ROW_PACK: only the direct scoring path is supported (dictionaries disabled or print_features requested)` and aborts. The original prints the parser features and continues. This is a deliberate guard in `research/final/203-cpi-memory/sources/crf.c:793`, the same one behind the earlier 119 report. It affects only this debugging API; default parsing, parsing with options, and every other API family were identical. Supporting it would mean keeping the unpacked scoring path for that mode, which is a source change needing a new candidate and full revalidation. That was not done.
- **Toponym is an upstream crash, not a regression.** `libpostal_is_toponym_duplicate` with default duplicate options crashes the original library too. In this input it happens on "16 pine cone drive, queensbury ny 12804" (line 191) checked against "calle sorolla 16 2, 28290 las matas" (line 190). Both builds behave identically up to and including the crash, but only the first 189 pairs were compared.
- Scope: one 4,356-address input, default and six fixed option sets, sequential pair checks. This is broad family coverage, not every possible argument or state. The fuzzy duplicate checks used whitespace tokens with score 1.0.
- These are fresh setup builds, not the archived 203 binaries. Default-parser byte parity of the archived build is covered by experiment 203.

Decision: record. API parity holds for 7 of 8 families; the feature-print debugging API remains a documented limitation of the candidate.
