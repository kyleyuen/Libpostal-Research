# Experiment 019 — General per-address costs (on 017 `u2`)

Date/time/timezone: started 2026-10-01 ~01:58 PDT (unattended overnight run; start time corrected)
Status: **complete.** Best: `h1` 3.85x golden-v1 / 3.86x novel / 3.79x floor / 3.74x golden-v3-osm; `p1` (= `h1` + packed weights) within noise of it (3.84x / 3.85x / 3.78x / **3.76x**) with peak footprint 2.18 GB vs 2.61 GB. All byte-identical.

## Hypothesis

After 016-018, the remaining time is spread over context fill (~3.2 µs), feature building (~2.7), lookups (~2.2), accumulation (~1.2), Viterbi (~1.0)
and per-address fixed costs (allocation, copies, strlen; ~8-10% of profile samples). The independent OSM set (short addresses) runs at 3.20x vs 3.42x on
golden-v1, consistent with fixed per-address costs weighing more there. General, exact changes that remove per-address and per-token work should lift
every country. Each change is a separate switch on top of `u2`; control `r0`-style rebuild; verified (dumps, frozen references, 2 x 2 M fuzz, plus
differential tests where a function is replaced) before timing on golden-v1, novel, floor and golden-v3-osm. Prediction: none.

## Log of work

**~01:58-02:14 (corrected: this heading first said "02:15-02:50", an estimate written before those times) — profile, five switches, builds, verification queued.**
- Profiles of `u2p` (u2 source, no LTO) on golden-v1 and golden-v3-osm (`raw/profile_u2p_*.txt`): accumulation ~1000-1150 samples, phrase search 1018-1343,
  lookups 896-1094, Viterbi 817-881, memmove 619-721, strlen ~370-450, memset 239-319 (crf.c lean context zeroing), free ~350 (per-address arrays),
  token normalization ~350-470 (`add_normalized_token` + utf8proc), fd_canon 127-222.
- `EXP_VITERBI_NEON` (`crf_context.c`): two target labels per NEON instruction, same per-label operations and order (sum ((prev+st)+tr), strict-greater
  first-max from -DBL_MAX/-1). Unit test `tools/viterbi_test.c`: 5,000,000 random steps with exact ties, -inf and NaN: 0 mismatches (`raw/viterbi_test.txt`).
- `EXP_TOKEN_ASCII` (`normalize.c` `add_normalized_token`): ASCII answers of `utf8proc_iterate`, `utf8proc_category`, `utf8_is_hyphen` from 128-entry tables
  filled by those same functions; the peek at the byte after the token keeps the library call when that byte is non-ASCII.
- `EXP_FD_INLINE` (012's inline hash tail/compare) retried under whole-library PGO/LTO.
- `EXP_REUSE` (`address_parser.c`): token array, tokenized string (+ its arrays and string copy), token labels and the response label/component arrays are
  kept and cleared per address instead of allocated and freed (requires EXP_RESPONSE_BLOCK). `leaks`: 0.
- Builds (whole-library PGO + full LTO, scanner -O1): `r1` control, `v1`, `n1`, `f1`, `a1` (v1+n1+f1), `u3` (reuse), `b1` (a1 + reuse). `a1` verified
  identical; others in the chain. `tools/run_timing.sh` (detached) times all verified builds on golden-v1, novel, floor and golden-v3-osm.
- Homebrew installs for experiment 020 (mimalloc 3, LLVM/clang 23.1.2, GCC 16.2); 020 builds hold the timing session via research/PAUSE_TIMING so they
  cannot overlap it.

**02:14-02:25 — more switches and a measurement.**
- `EXP_NAME_LEN` (`features.c` `feature_array_add_named` + macro in `address_parser.c`): the feature name's length comes from `sizeof(literal) - 1` for string
  literals and `strlen` for `char *` names (selected with `__builtin_types_compatible_p`); same bytes and strip rule as EXP_FAST_JOIN. Build `b2` = `b1` + this.
- `EXP_NORM_HANGUL` (experiment 022 item, `address_parser.c` table builder): Hangul syllables U+AC00-D7A3 admitted to the T5 table (jamo stay excluded; NFC can
  only append a following trailing jamo; nothing composes backward into a syllable). Table 42,532 -> 53,704 codepoints. Differential test (016 `t5_diff`,
  seed 3, corpus golden-v1 + novel + golden-v3-osm): 23,133,355 fast-path answers, 0 mismatches (`raw/t5_hangul_diff.txt`). Build `h1` = `b2` + this.
- `EXP_ROW_PACK` (experiment 021 item, `crf.c`): weights read from the model file straight into packed 10-byte records {double, uint16 class} (two
  chunked passes with fseek; the parallel indices/data arrays are never allocated); accumulation and prefetch use them; non-direct scoring paths abort.
  Build `p1` = `h1` + this: 300-address sample identical, 0 leaks, peak footprint 1.956 GB vs 2.087 GB for `b2` in the dumper (-130 MB).
- Measurement (`EXP_SEARCH_STATS`, build `sst`, `raw/search_stats.txt`): golden-v1 220,006 phrase-search tokens start from a fresh state; 93,384 (42%) fall off
  inside the token: after 0 / 1 / 2 / >=3 bytes = 1,672 / 32,245 / 31,852 / 29,615 (OSM set similar). T6's 2-byte filter covers the first ~34 K; a full
  prefix filter could skip ~60 K more short walks (estimated +1-2%, not built yet).
- Verified so far: `r1`, `v1`, `n1`, `f1`, `a1`, `u3`, `b1`, `b2` byte-identical; `h1`, `p1` in the chain; timing waits for them and for 020's builds.

**02:25-02:35 — q1 and a contaminated trial (recorded before any results were read).**
- `EXP_ROLE_IDS` (experiment 023 item; `crf.c` + `address_parser.c`): at load, for every vocabulary slot and each single-word role (state word / prev word /
  next word / first word; transition word / prev word) the integer table's answer is stored; emission pushes that feature id (flag bit 63) instead of a
  key, and the lookup pass uses it directly. Build `q1` = `p1` + this: byte-identical (dumps, frozen references, 2 x 2 M fuzz), 0 leaks; harness peak
  footprint 2.244 GB (`p1` 2.179 GB, original 2.613 GB). Built and verified inside its own pause (`research/PAUSE_TIMING_q1`; the gate now honours any
  `PAUSE_TIMING*` file). Not in this session's variant list (fixed at launch); to be timed separately.
- **Contamination:** a leftover queued job from experiment 020 (believed dead) released `research/PAUSE_TIMING` at 02:28:34 while the 020 builds `x0`,
  `x1`, `x3` were still being verified (until ~02:33). This session's golden-v1 trial-1 runs for every variant (started 02:29-02:33) overlapped that CPU
  load. They will be renamed `*.excluded` and rerun under the same names after the session; the summary is recomputed then. Cause: an over-broad `pkill`
  earlier killed the 020 build chain but not this waiter, and a manually written log line then woke it.


## Results (AC, launcher start 02:23, runs from 02:29, DONE 03:40, reruns done 03:44 — from marker files; 11 golden-v1 trial-1 runs that overlapped 020 verification were excluded and rerun, `raw/timing/RERUNS.txt`)

Speedup vs the same-session original by median (all estimators agree; `raw/timing/A-summary.md`, `floor-summary.md`, `v3-summary.md`):

| build | adds | golden-v1 | novel | floor | golden-v3-osm |
|---|---|---:|---:|---:|---:|
| 017 `u2` | — | 3.450x | 3.430x | 3.367x | 3.204x |
| `r1` (control: u2 rebuilt) | — | 3.447x | 3.441x | 3.387x | 3.211x |
| `v1` | NEON Viterbi | 3.444x | 3.438x | 3.368x | 3.210x |
| `n1` | ASCII token normalization | 3.450x | 3.451x | 3.388x | 3.234x |
| `f1` | inline dictionary hash/compare (012 iA, retried) | 3.591x | 3.570x | 3.494x | 3.366x |
| `a1` | v1 + n1 + f1 | 3.559x | 3.555x | 3.483x | 3.346x |
| `u3` | reuse per-address objects | 3.691x | 3.694x | 3.640x | 3.540x |
| `b1` | a1 + reuse | 3.823x | 3.838x | 3.763x | 3.691x |
| `b2` | b1 + compile-time name lengths | 3.848x | 3.843x | 3.785x | 3.718x |
| **`h1`** | b2 + Hangul syllables in the table | **3.854x** | **3.863x** | **3.786x** | 3.744x |
| **`p1`** | h1 + packed weight records | 3.839x | 3.846x | 3.779x | **3.760x** |

Original µs/address this session: golden-v1 33.93, novel 34.51, floor 32.74, golden-v3-osm 21.30; `h1` 8.81 / 8.93 / 8.65 / 5.69.

## Interpretation and decision

- The two big levers were **reusing per-address objects** (+7.1% alone over `r1`) and **the inline hash/compare** (+4.2% alone) — the latter was
  slower in 012 under plain -O2 but helps under whole-library PGO + LTO. NEON Viterbi and ASCII token normalization are neutral (exact, kept off the
  critical list). Name lengths +0.7%; Hangul raises the OSM set +0.7% (Korea), packed weights are speed-neutral but cut peak memory by ~360 MB.
- **Accepted: `p1` is the new best candidate** — within noise of `h1` on speed and far lower memory (harness peak 2.179 GB vs 2.613 GB original),
  3.84x / 3.85x / 3.78x / 3.76x. `q1` (precomputed role ids) is verified and is timed in the next session together with the 020 compiler/allocator builds.

## Follow-up builds after the session (03:45-03:56; corrected: first written as "03:45-04:05", an estimate ahead of the clock), timed in experiment 020's session 2

- Profile of `p1p` (p1 source, no LTO; `raw/profile_p1p_v1.txt`): phrase search 1494, lookups 1324, accumulation 1181, Viterbi 593, memmove 433,
  `fd_find_slot` 398 (211 of them from `ap_canon_part` on phrase-valued neighbours), memset 336, `graph_has_edge` 303.
- `q2` = `q1` + `EXP_WORD_MEMO` (016 T7) + `EXP_WM_CANON`: the canon of a memoized phrase result is computed once per memo entry (the memo owns a stable
  copy of the bytes for the whole address). Byte-identical (dumps, frozen references, 2 x 2 M fuzz), 0 leaks.
- `g1` = `q2` + `EXP_PHRASE_PREFILTER` (016 T6, which was not in the b/p line) + `EXP_PREFIX34` (experiment 024): exact sets of every 3- and 4-byte prefix
  walkable from each search's start node (double-array paths to depth 4, leaf tails appended) let fresh-state tokens whose first 3/4 bytes start no path
  skip the walk (the 020/019 measurement: ~60 K such walks fall off after >= 2 bytes on golden-v1). Verify build `g1v` (every search run with and without
  the filters): golden-v1 38,589, novel 38,589, golden-v3-osm 34,215 searches, 0 mismatches (`raw/g1v_verify_datasets.txt`); fuzz and full checks running.
- `g2` = `g1` + `EXP_INT_KEYS_PAIRS` (pair features through the integer table, retried under the faster pipeline); harness peak 2.333 GB; verification queued.
- Measurement (`EXP_DUP_STATS`, build `dst`, `raw/dup_stats.txt`, `raw/string_key_names.txt`): on the development split 357,661 of 562,743 lookups still use
  the string path (64%), 52,232 of them repeat an earlier string key of the same address (14.6%); 205,082 are integer/resolved. By name (2,000
  development addresses): phrase type+phrase 12,259, prev word+word 9,623, word+next word 9,543, phrase 6,207, bias 5,289, word (mostly n-grams) 4,429,
  commonly city 4,366, unambiguous phrase type(+phrase) 3,852 each, postcode have/no context 2,887 / 2,174. (The first `dst` rebuild failed on an
  undeclared `ptrdiff_t`; the histogram was taken from the fixed rebuild.)
