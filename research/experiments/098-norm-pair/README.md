# Experiment 098 — standard and admin token normalization in one scan (EXP_NORM_PAIR)

Date/time/timezone: created 2026-10-03T21:49:20-0700 (`raw/created.txt`), America/Los_Angeles. Status: **accepted** (unseen golden-v8 in 102: +1.5% vs vmx, every round)

## Hypothesis and change
Context fill normalizes every token twice: with the standard options (DELETE_FINAL_PERIOD | DELETE_ACRONYM_PERIODS | REPLACE_DIGITS = 0x8C) and with the admin options (the same without REPLACE_DIGITS = 0x0C). `EXP_ADMIN_SHARE` (011) already reuses the standard result for ASCII tokens without digits. Every other token (digits, non-ASCII) ran the full `add_normalized_token` loop twice, and that loop computes the hyphen flag and the next character's category on every character. With these two option sets none of those values can affect the output: the hyphen, split, possessive and apostrophe branches are off, and the lookahead feeds only them.
Change (`sources/normalize.c` `add_normalized_token_pair`, call site in `sources/address_parser.c` context fill; diffs `raw/normalize.diff`, `raw/address_parser.diff`): one scan writes both outputs.
- A number character goes to the standard form as DIGIT_CHAR and to the admin form unchanged.
- A full stop at the last byte ends both forms; a full stop in an ACRONYM token is dropped from both.
- Every other character is appended to both.
- All-letter and all-digit ASCII tokens take a one-pass fast path.
- The iterate call (full `len`), the `char_len <= 0` stop and the `nt_tables()` initialization guard are kept as in the original.
Base: vmx (091 sources, `raw/base_sources_vmx.sha256`); switches `raw/switches_npr.txt` = vmx + `-DEXP_NORM_PAIR`.

## Reproduction
Commands with timestamps: `raw/commands.txt`. Build: `EXTRA_SW="$(cat raw/switches_npr.txt)" tools/build_cc.sh npr clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o` (PGO on golden-v1 development, as every build since 020). The first build attempt failed: a prototype placed in a copied `normalize.h` was shadowed by the original header that `address_parser.h` includes. The prototype was moved to `address_parser.c`; the failed attempt's logs are kept as `raw/build_npr_attempt1.*`. Hashes: `raw/key_hashes.txt`, `artifacts/*/hashes.txt`.

## Correctness
- Differential unit test (`tools/norm_pair_test.c`, `raw/norm_pair_test.stdout`) compares the pair function against two `normalize_token` calls. The input set has 145 elements: ASCII 1–127, fullwidth/Arabic-Indic/Devanagari digits, ², ½, Ⅻ, multibyte letters, fullwidth and ideographic stops, a Unicode hyphen, truncated UTF-8 and a stray continuation byte. Results:
  - Every 1- and 2-element string × token types × 5 trailing contexts (bytes after `token.len`): 3,668,500 cases, **0 mismatches**.
  - Every 3-element string over 19 elements × 130 types: 891,670 cases, **0 mismatches**.
  - 20,000,000 random tokens: **0 mismatches**.
  - A planted mutation (ACRONYM admin form normalized with the standard options) is detected: 4,416 / 4,557 / 541 mismatches (`raw/norm_pair_test_mutant.stdout`).
- `tools/verify.sh npr` (`raw/verify_npr/summary.txt`): golden-v1 sha 45574ee2…, novel 806b8a81…, OSM, unseen v4, both frozen references, fuzz seeds 1 and 2 (2M each): **all identical**.

## Screen (vs vmx; sequential, rotating order, 20 passes, 6 rounds; 2026-10-03T21:52:26 → 21:57:39 -0700; golden-v8 Overpass fetch running concurrently, network only)
`raw/screen_analyze.stdout` (median of passes 2–20 per run, µs/address):
    v1 vmx [6.419, 6.398, 6.394, 6.427, 6.403, 6.395] median 6.4
    v1 npr [6.303, 6.355, 6.344, 6.319, 6.317, 6.36] median 6.332
    v1 npr speedup vs vmx per round [1.0184, 1.0068, 1.0079, 1.0171, 1.0136, 1.0055] median 1.0108
    osm vmx [4.087, 4.117, 4.241, 4.247, 4.039, 4.107] median 4.112
    osm npr [4.083, 4.036, 4.169, 4.049, 4.008, 4.191] median 4.066
    osm npr speedup vs vmx per round [1.001, 1.0201, 1.0173, 1.0489, 1.0077, 0.98] median 1.0125
+1.1% golden-v1 (6/6 rounds), +1.25% OSM (5/6 rounds; OSM rounds are noisy).

## Mechanism (stage timers; gain ≈ 1%, so required)
Stats builds `vst` (vmx) and `nst` (npr) from `sources_stats/` (073's timer around the per-token normalization loop, ss_t[8]); harness `tools/bench_split_vt.c` (090's); 3 alternating runs each on development (025 `development.tsv`) and OSM (`raw/split_*`, `raw/split_analyze.stdout`). Medians, cold/warm µs per address:

| | dev vmx | dev npr | OSM vmx | OSM npr |
|---|---:|---:|---:|---:|
| normalization loop | 0.235 / 0.163 | 0.147 / 0.097 | 0.187 / 0.136 | 0.120 / 0.083 |
| context fill | 2.138 / 0.534 | 2.029 / 0.461 | 1.275 / 0.473 | 1.201 / 0.417 |
| features, lookups | unchanged | | unchanged | |
| total parse | 6.748 / 2.936 | 6.629 / 2.851 | 4.449 / 2.523 | 4.388 / 2.466 |

The saving is in the normalization loop and nowhere else, about 0.07–0.09 µs, consistent with the screen.

## Decision
Screen-positive with a confirmed mechanism. Per the unseen-data rule, acceptance requires re-measurement on golden-v8 (fetch started 2026-10-03T21:46:47-0700 per `profiling/golden-v8-osm-unseen/fetch_start.txt`). Not yet accepted.

Update: Accepted after the unseen check (102: npr vs vmx +1.5%, per-round 1.002–1.022, outputs identical on 9,981 addresses). Part of deliverable psf (103).
