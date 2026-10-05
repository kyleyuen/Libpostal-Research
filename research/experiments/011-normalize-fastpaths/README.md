# Experiment 011 — Exact ASCII fast path for string normalization; shared admin token normalization (on top of 010 `g`)

Date: 2026-09-29 PDT (session 20:07-20:40; fuzz and verification 20:45-21:10). Status: **accepted (variant `h`: 2.66x golden-v1 / 2.71x novel vs the original baseline; byte-identical; single thread; peak memory lower)**
Run unattended (laptop caffeinated, AC gate with wait-and-retry).

## Findings that motivated it
- Profile (008): whole-string normalization is ~2.5 us per address (about 16% of the current time) and `context_fill` normalizes every token twice.
- `address_parser_normalize_string` = `normalize_string_latin(COMPOSE | LOWERCASE | SIMPLE_LATIN_ASCII)`: transliterate with `latin-ascii-simple`, then a UTF-8 normalize.
- `address_parser_context_fill` normalizes each token with the standard options and again with the admin options. `ADMIN = STANDARD ^ NORMALIZE_TOKEN_REPLACE_DIGITS`, and that flag is used in exactly one place (`normalize.c:457`, guarded by `is_number`).

## Changes (each behind its own switch)
- **EXP_ASCII_NORM** (`address_parser.c`): if the string is non-empty and consists only of bytes 0x20-0x7E other than `&`, return its ASCII lowercase (a fresh `malloc`, as before); otherwise call the original.
- **EXP_ADMIN_SHARE** (`address_parser.c`): for a token with only ASCII bytes and no digit, reuse the standard normalization result for the admin arrays instead of normalizing again.
Base = 010 `g` without the arena prefetch (`g2`, the control). Variants: `h1` = ASCII norm, `h2` = admin share, `h` = both.

## Why the fast path is exact (proof by inspection of the rule tables, then tested)
Enumerated every rule of the loaded transliteration trie for the `latin-ascii-simple` transliterator (`tools/trans_rules.c`, `raw/trans_rules.txt`):
```
transliterator latin-ascii-simple: 2 steps
 step 64: name=html-escape type=transform
  transliterator html-escape: 1 steps
   step 136: name=0 type=ruleset
    ruleset html-escape|0: keys scanned=516738 under-namespace=65788 ASCII-only rule sources=65788
    first bytes of ASCII-only sources: &(65788)
    distinct first bytes over ALL sources (incl. non-ASCII): 1 : 0x26(65788)
 step 65: name=1 type=ruleset
  ruleset latin-ascii-simple|1: keys scanned=516738 under-namespace=138 ASCII-only rule sources=0
  first bytes of ASCII-only sources:
  distinct first bytes over ALL sources (incl. non-ASCII): 6 : 0xc2(2) 0xc3(2) 0xcb(6) 0xe2(36) 0xe3(13) 0xef(79)
```
- Step 1, `html-escape`: 65,788 rules, all ASCII, **every one starts with `&`**.
- Step 2, ruleset `1`: 138 rules whose first bytes are only UTF-8 lead bytes 0xC2, 0xC3, 0xCB, 0xE2, 0xE3, 0xEF.
So no transliteration rule can start at any printable ASCII byte other than `&`; for such a string transliteration leaves it unchanged, and the UTF-8 step (compose + lowercase) only maps A-Z to a-z. Control characters ARE changed by the library (0x01-0x08, 0x0e-0x1f and 0x7f are dropped, tab becomes a space, 0x0b-0x0d become newlines) and `&amp;`-style entities are rewritten, so those bytes are outside the fast path's domain and fall back to the original code.
Differential tests against the library (`tools/ascii_norm_test_D.c`): **839,514 exhaustive strings of length 1-3 over the domain, 60,000,000 random strings of length 4-96, and 7,710 corpus lines: 0 mismatches** (`raw/ascii_norm_test_D.stdout`). An earlier, wider test that included control bytes and `&` found the mismatches described above (`raw/ascii_norm_test.stdout`, kept).
The admin-share claim was tested separately (`tools/admin_share_test.c`): standard vs admin normalization on ASCII digit-free tokens, all 130 token types x every 1-char string and 19 types x every 2-char string (275,301 cases, control characters included) plus 20,000,000 random tokens: **0 mismatches**.

## Correctness (hard constraints)
- **Byte-identical**: `g2`, `h1`, `h2`, `h` match the baseline on golden-v1/all (12,863; includes the corpus line with `&amp;`) and the novel set (12,863).
- **End-to-end differential fuzz** (`research/tools/fuzz_pipeline.c`, `raw/fuzz/`): 2 seeds x 2,000,000 mutated addresses (random bytes, byte deletions/swaps, non-ASCII characters, HTML entities, control characters, digits, casing, truncation, repetition to long inputs, concatenation of two addresses),
  baseline vs `h`: **every per-input result hash identical** (about 1.79M valid inputs parsed per seed; 211k per seed were invalid UTF-8 and skipped, see below).
- **Single thread**: `ps -M` = 1 thread at all 6 samples; CPU seconds 25.08 vs wall 25.09.
- **Memory**: peak footprint 2,556,758,200 B (baseline 2,613 MB, -2.2%); steady-state footprint 2,438 MB (`footprint -p`).
- Pre-existing baseline bug (not ours, not changed): the original library **segfaults on some invalid UTF-8 inputs** (`transliterate` logs "invalid UTF-8" and the caller then crashes). Such inputs have no defined output to preserve; the fuzzer therefore only feeds valid UTF-8 and reports the invalid ones as skipped.

## Results (controlled unattended session; baseline, 010 `g`, `g2`, `h1`, `h2`, `h`; 5 processes x 19 timed passes per variant per dataset)
| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 95 | 2.93 | nan | 100% | 36.69 | 36.12 | 36.69 | nan (0) | 36.91 |
| novel | g | 95 | 6.69 | nan | 100% | 15.34 | 15.14 | 15.34 | nan (0) | 15.46 |
| novel | g2 | 95 | 6.70 | nan | 100% | 15.36 | 15.21 | 15.35 | nan (0) | 15.50 |
| novel | h1 | 95 | 6.74 | nan | 99% | 13.75 | 13.60 | 13.75 | 13.68 (1) | 13.86 |
| novel | h2 | 95 | 6.64 | nan | 100% | 15.18 | 15.03 | 15.17 | nan (0) | 15.22 |
| novel | h | 95 | 6.58 | nan | 99% | 13.56 | 13.45 | 13.55 | 13.38 (1) | 13.69 |
| v1-all | baseline | 95 | 2.91 | nan | 100% | 36.06 | 35.51 | 36.05 | nan (0) | 36.05 |
| v1-all | g | 95 | 6.76 | nan | 98% | 15.22 | 15.10 | 15.21 | 15.16 (2) | 15.29 |
| v1-all | g2 | 95 | 6.71 | nan | 100% | 15.25 | 15.05 | 15.25 | nan (0) | 15.30 |
| v1-all | h1 | 95 | 6.66 | nan | 99% | 13.78 | 13.64 | 13.77 | 13.65 (1) | 13.90 |
| v1-all | h2 | 95 | 6.66 | nan | 100% | 15.09 | 14.90 | 15.09 | nan (0) | 15.25 |
| v1-all | h | 95 | 6.72 | nan | 99% | 13.55 | 13.40 | 13.55 | 13.52 (1) | 13.76 |


Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | g | 2.392x | 2.385x | 2.392x | nanx | 2.387x |
| novel | g2 | 2.389x | 2.375x | 2.390x | nanx | 2.381x |
| novel | h1 | 2.669x | 2.655x | 2.669x | nanx | 2.664x |
| novel | h2 | 2.418x | 2.403x | 2.418x | nanx | 2.425x |
| novel | h | 2.707x | 2.686x | 2.706x | nanx | 2.697x |
| v1-all | g | 2.370x | 2.352x | 2.369x | nanx | 2.357x |
| v1-all | g2 | 2.364x | 2.360x | 2.364x | nanx | 2.357x |
| v1-all | h1 | 2.617x | 2.604x | 2.617x | nanx | 2.593x |
| v1-all | h2 | 2.389x | 2.384x | 2.388x | nanx | 2.364x |
| v1-all | h | 2.661x | 2.651x | 2.660x | nanx | 2.619x |



Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   g     : countries=17 min 2.221 (lt) median 2.377 max 2.638 (ru); below 1.05x: 0
  novel   g2    : countries=17 min 2.211 (lt) median 2.378 max 2.637 (ru); below 1.05x: 0
  novel   h1    : countries=17 min 2.259 (lt) median 2.642 max 2.862 (us); below 1.05x: 0
  novel   h2    : countries=17 min 2.238 (lt) median 2.407 max 2.643 (ru); below 1.05x: 0
  novel   h     : countries=17 min 2.268 (lt) median 2.651 max 2.918 (us); below 1.05x: 0
  v1-all  g     : countries=17 min 2.092 (lt) median 2.317 max 2.518 (ru); below 1.05x: 0
  v1-all  g2    : countries=17 min 2.098 (lt) median 2.342 max 2.532 (ru); below 1.05x: 0
  v1-all  h1    : countries=17 min 2.125 (lt) median 2.536 max 2.915 (gb); below 1.05x: 0
  v1-all  h2    : countries=17 min 2.115 (lt) median 2.372 max 2.548 (ru); below 1.05x: 0
  v1-all  h     : countries=17 min 2.161 (lt) median 2.559 max 2.994 (gb); below 1.05x: 0


Peak memory footprint per process (bytes) by variant (all processes, both datasets):

| variant | processes | min | median | max | max RSS max |
|---|---:|---:|---:|---:|---:|
| baseline | 10 | 2,612,955,344 | 2,613,209,308 | 2,613,840,104 | 2,614,673,408 |
| g | 10 | 2,557,233,312 | 2,557,511,852 | 2,558,462,112 | 2,559,328,256 |
| g2 | 10 | 2,556,561,592 | 2,557,405,368 | 2,558,707,872 | 2,559,541,248 |
| h1 | 10 | 2,557,102,240 | 2,557,192,376 | 2,557,937,848 | 2,558,754,816 |
| h2 | 10 | 2,556,561,592 | 2,557,282,464 | 2,559,281,336 | 2,560,131,072 |
| h | 10 | 2,557,102,240 | 2,557,905,056 | 2,558,544,056 | 2,559,377,408 |



Gains over the control `g2` (golden-v1 / novel, by median): `h1` +10.7% / +11.7%, `h2` +1.1% / +1.2%, `h` +12.6% / +13.2%. Frozen harness on the full corpus: 74,341 addr/s (13.45 us/address) for `h`.
Cold first pass (model warmed on the other dataset) agrees with the warm passes (2.62x / 2.70x). Per-country speedup of `h`: golden-v1 min 2.16x (lt), median 2.56x, max 2.99x (gb); novel min 2.27x, median 2.65x, max 2.92x (us).

## Dependence on the input mix (important for a new dataset)
The ASCII fast path only applies to inputs made of printable ASCII without `&`: **58.0% of golden-v1 (57.9% development, 58.4% validation) and 59.9% of the novel set**.
Non-ASCII inputs use the original path and gain only from the other changes. From `g2` -> `h1` (-1.47 us average with 58% eligible inputs) the saving per eligible address is about 2.5 us, so the expected speed of `h` is roughly 15.25 us (2.36x) at 0% eligible inputs and 12.7 us (2.84x) at 100% (estimate, linear in the eligible share).

## Limitations
- The fast-path exactness argument rests on the rule enumeration plus differential tests, not on a formal proof of the UTF-8 normalizer; it is checked exhaustively for short strings and by 60M random longer ones.
- Setup time is unchanged from 010 (~6.6 s, baseline 2.9 s).
- Shared, loaded machine (about 100% of passes saw an involuntary context switch); estimators agree within ~1%.
- The validation split alone was timed separately with the frozen `parser_profile` harness (below).

## Validation split alone (frozen `parser_profile` harness, AC gate OK, 7 rounds + burn-in, alternating; `raw/validation-controlled-summary.txt`)
```
validation split (2,555 addresses, 58.4% fast-path eligible), frozen parser_profile harness, AC gate on, 7 rounds + burn-in, alternating order
baseline  median 27,760 addr/s (36.02 us/addr)  min 26,459 max 28,276  (max-min)/median 6.5%
h         median 75,024 addr/s (13.33 us/addr)  min 74,431 max 75,408  (max-min)/median 1.3%
speedup by medians 2.703x ; paired median 2.705x (min 2.632 max 2.850); h faster in 7/7 rounds
```

## Decision
**Accepted** as the new best (`h`). Ledger, report and activity log updated: yes.
