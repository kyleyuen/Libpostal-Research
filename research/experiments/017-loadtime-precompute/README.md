# Experiment 017 — Spend load/build time to save memory and runtime (PLANNED, nothing built)

Date/time/timezone: recorded 2026-09-30 (plan only)
Status: **steps 1-2 complete.** Best `u2`: **3.42x golden-v1, 3.42x novel, 3.35x floor** (same-session original), byte-identical, one thread, peak 2.539 GB. Items 1-5 of the original plan below remain open.

## Idea

Load time and build (compile/preprocess) time may grow (e.g. load up to ~20 s or more) if that removes per-address work or memory. Startup must be
reported separately; confirm with the evaluator that startup is not part of the speed measurement. Limits stay: byte-identical output, one thread,
peak and steady memory not above the original (current headroom ~75 MB), tables derived from the model/code only.

## Candidates, in intended order

1. **Weight rows next to their keys (audit C1)** — rebuild the CSR weight storage at load; expected to free ~120 MB (indices 4 B -> 1 B class ids,
   no separate `indptr`) and cut reads per lookup. Creates the memory room the next items need.
2. **Per-word records for frequent words** (ranked by the model's own vocabulary counts) — precompute per word what is searched per token today
   (word id, prefix/suffix dictionary matches, phrase-start bits) so one read replaces several searches.
3. **Precomputed partial weight sums** (ALGORITHM_RESEARCH 3.1) — exact only for same-order prefixes; 014 found ~25% of plain tokens qualify, so modest.
4. **Larger normalization table** — extend 016 T5 beyond the BMP if the floor still pays for normalization.
5. **Move all load-time construction offline** — build dictionaries, integer-key tables and normalization tables once into files that are `mmap`ed
   at start-up, bringing load time back to (or below) the original ~3 s at the same runtime; allows heavier precomputation.

## Open questions before starting

- Does the evaluation time startup? (Decides whether load time is free.)
- 016 per-stage timing (T2): which stages are left after T4-T8, and how much each would gain.

## Log of work

**Profile first (2026-09-30 23:55, AC).** `sample` of 016 `t45` (same source as 018 `c2s1`, no LTO so functions stay visible) for 15 s of warm passes on
golden-v1 (`raw/profile_t45.txt`). Top of stack (samples of ~15,000): `trie_search_tokens_from_index` 1643, `crf_dict_lookup_array` 1401,
`crf_tagger_score` (accumulation) 1204, `memmove` 850, `crf_context_viterbi` 834, `feature_array_add` 425, `strlen` 378 (+96 stub), `memset` 322,
`memcmp` 307, `graph_has_edge` 307, **`fd_canon` 295**, `scan_token` 274, `ap_vm_get` 261. The `fd_canon` samples come from `address_parser.c`
1774/1802 — the `prev word` / `next word` emissions — where the value is not a memoized token: mostly the constants `UNKNOWN` / `UNKNOWN_NUMERIC`
substituted for out-of-vocabulary neighbours (numbers, postcodes), each costing a vocabulary hash lookup.

**Step 1 — `EXP_CANON_CONST`.** canon() of the two constant strings is computed once at load; `ap_canon_part` recognises them by content
(`strcmp`, after a first-byte check) and returns the cached value. Exact: canon() is a function of the string's bytes only.
Build `u1` (018 `c2s1` flags: whole-library PGO + full LTO, scanner -O1 from the cache) — byte-identical (dumps, frozen references, 2 x 2 M fuzz).
Control `r0` = the same sources and flags without the new switches (rebuild control).

**Step 2 — `EXP_RESPONSE_BLOCK` (audit R1).** Profile attribution: `memmove`/`strlen` callers include `cstring_array_to_strings` (one `strdup` per label and
component) and the per-string frees in `libpostal_address_parser_response_destroy`. The response's two pointer arrays and all string bytes are now one
allocation; destroy frees that block (and the struct). Both response creation sites (single-phrase bypass and prediction) use it. Same strings, same
order; callers must free through the documented destroy function (unchanged API). Build `u2` = step 1 + step 2. `leaks --atExit` on 300 addresses:
`u2` 0 leaks; `r0` 1 pre-existing 128-byte leak (`raw/leaks_*.txt`). The first `u2` edit failed on whitespace and that build contained no step-2 code;
it was rebuilt (`raw/compile_times.txt`).
`tools/run_timing.sh` (detached) times original, 018 `c2s1`, `r0`, `u1`, `u2` once `u2` and `r0` are verified.

## Results (AC, 2026-09-30 23:53 - 2026-10-01 00:16 PDT; clean: no waits, no battery runs)

Session A (`run_unattended.sh`, 5 processes x 18 warm passes, rotating; `raw/timing/A-summary.md`), speedup vs the same-session original by median:

| build | golden-v1 µs | vs original | novel µs | vs original | floor µs | vs original |
|---|---:|---:|---:|---:|---:|---:|
| original | 33.64 | 1.000x | 34.24 | 1.000x | 32.42 | 1.000x |
| 018 `c2s1` | 10.03 | 3.353x | 10.14 | 3.376x | 9.81 | 3.306x |
| `r0` (control: same sources/flags, no new switch) | 10.00 | 3.366x | 10.19 | 3.360x | 9.75 | 3.325x |
| `u1` (step 1, constant canon cache) | 9.97 | 3.375x | 10.16 | 3.372x | 9.77 | 3.317x |
| **`u2` (steps 1 + 2, one-block response)** | **9.85** | **3.417x** | **10.03** | **3.415x** | **9.68** | **3.351x** |

vs `r0`: `u1` +0.3% / +0.4%; `u2` +1.5% / +1.6% / floor +0.8%. Peak footprint: original 2,613,184,744; `r0` 2,538,768,568; `u2` 2,538,784,976.
The original ran faster in this session (33.64 µs) than in 018 (34.42 µs); speedups are only compared within a session.

## Interpretation and decision

Step 1 saved less than its 2% profile share (+0.3%); step 2 (one allocation instead of 2 + n + n per response) gave +1.2% on its own.
**Accepted: `u2` is the new best candidate** (3.42x / 3.42x / floor 3.35x). Remaining to 3.5x: ~2.4% (~0.24 µs/address) on the average.
Open: the original plan's items (weight rows inline, per-word records, offline tables) and other allocation work (R2 arenas) for the last 2-3%.

## Third, independent dataset (golden-v3-osm), 2026-10-01 01:55-02:05 PDT

`profiling/golden-v3-osm/` (11,405 OpenStreetMap addresses, 49 countries, 45% non-ASCII; independent source, built after all tuning).
Exactness: `u2` output byte-identical to the original library's on all 11,405 (`raw/v3check/summary.txt`). Timing (`tools/run_v3.sh`, AC, 5 processes
each, rotating order, clean: no waits, no battery runs, no outlier passes; `raw/v3/summary.md`):

| build | µs/address | vs original | vs `r0` |
|---|---:|---:|---:|
| original | 21.17 | 1.000x | |
| `r0` | 6.71 | 3.154x | 1.000x |
| **`u2`** | **6.61** | **3.202x** | 1.015x |

Peak footprint: original 2,612,955,368; `u2` 2,538,424,504. Per country (49): median 3.25x; lowest Korea 2.38x (Hangul is excluded from the 016 T5
table), Taiwan 2.70x, Lithuania 2.79x; highest Germany 3.50x, Belgium 3.52x, Greece 3.52x, Chile 3.55x. The set's addresses are shorter than golden-v1's
(5.1 vs 5.6 words), so fixed per-address costs weigh more. **On data never used for any decision, the best build is 3.20x.**
