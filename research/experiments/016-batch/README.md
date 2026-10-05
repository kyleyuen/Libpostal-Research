# Experiment 016 — Batch of measurements and exact optimizations (tests 1-11), on top of 015 `k1`

Date/time/timezone: started 2026-09-30 15:45 PDT
Status: **complete.** Best build `pgolto` (T4 + T5 + T6 + T8 + PGO/LTO): **3.26x golden-v1, 3.27x novel, 3.24x floor** vs the original, byte-identical, one thread, peak 2.539 GB. T4+T5 (normalization) and T11 (PGO+LTO) carry the gain; T6, T7, T8 show no measurable gain.

## Plan and hypotheses (one switch per optimization so each is attributable)

Base: 015 `k1` (= 011 `h` + `EXP_VOCAB_MEMO` + `EXP_INT_KEYS`). Sources copied from `015-integer-keys/candidate/` plus the untouched library files
`transliterate.c`, `normalize.c`, `crf_context.c`, `libpostal.c`, `tokens.c` (start hashes `raw/start_sources.sha256`).
Control: `k0` = the 016 sources built with exactly `k1`'s switches (rebuild control). Every optimization is a switch added on top of `k1`.

| Test | Kind | Switch | Hypothesis / why | Needs AC |
|---|---|---|---|---|
| T1 | measurement | — | Is the 3.3 µs non-ASCII gap normalization, or do non-ASCII addresses have more tokens/features? Counts only. | no |
| T2 | measurement | — | Real µs per stage on `k1` (normalize, tokenize, context fill, features, lookups, scoring, Viterbi, response) on full + floor sets. | yes |
| T3 | measurement | — | 20-round paired `h0` vs `k1` to confirm 015's +2.1-2.6%. | yes |
| T4 | optimization | `EXP_TRANS_SKIP` | N1-N3: skip transliteration steps whose rules provably cannot fire; stop recomputing per-call constants. | build no / time yes |
| T5 | optimization | `EXP_NORM_TABLE` | N5/N6: one-pass table normalization with library fallback; only if T1/T2 show normalization is large. | same |
| T6 | optimization | `EXP_PHRASE_PREFILTER` | P1/P2: first-two-byte prefilters so tokens that cannot start a phrase skip the trie walk. | same |
| T7 | optimization | `EXP_WORD_MEMO` | F1/F2: word descriptors once per token, phrase features once per phrase. | same |
| T8 | optimization | `EXP_ALLOC_LEAN` | R1/R2/C2 (+R3): fewer per-address allocations, clear only touched score cells. | same |
| T9 | optimization | `EXP_ROW_INLINE` | C1: weight rows stored next to their keys (fewer reads, frees memory). | same |
| T10 | optimization | `EXP_WORD_RECORD` | Per-word records for frequent words (coverage measured first). | same |
| T11 | build | PGO/LTO | Compiler profile-guided + link-time optimization trained on development split only. | same |

Acceptance per optimization (METHOD.md): byte-identical on golden-v1 and novel (dump + frozen references + negative control), 2 x 2 M fuzz identical,
single thread, peak and steady footprint not above the baseline, then timed in the controlled session against baseline, `k0`, `k1`.
Any item that cannot be proven exact is recorded and dropped, not forced. Predictions: none.

## Log of work (filled as it happens)

**15:45-16:05 — setup, control, T1.** Sources copied; `tools/build.sh` builds `k0` (control, `k1`'s switches) and `s` (stats build: `EXP_STAGE_STATS` timers
and counters in `address_parser_parse` and `crf_tagger_score_direct`, driver `tools/stage_bench.c`); log `raw/build1.log`. `k0` byte-identical on golden-v1
(`45574ee2...`) and novel (`806b8a81...`) (`raw/outputs_k0_*`).
**T1 result (counts; valid on battery)** — `raw/t1_counts_*.txt`, 1 pass each after a warm-up pass:

| subset | addresses | bytes | tokens | state features | transition features |
|---|---:|---:|---:|---:|---:|
| floor (non-ASCII / & / control) | 5,404 | 42.71 | 5.949 | 45.78 | 8.67 |
| ASCII | 7,459 | 36.38 | 5.833 | 45.94 | 8.78 |

Non-ASCII addresses do not carry more model work (tokens +2%, features equal); they are only longer in bytes. The same runs' stage times (battery,
INDICATIVE ONLY, not a result; to be repeated on AC in T2): normalize 2.83 vs 0.05 µs, total 14.39 vs 11.31 µs, i.e. ~2.8 of the ~3.1 µs gap is
whole-string normalization. Decision: T4/T5 (normalization) are justified for the floor.

**16:05-16:50 — T4, T8, T6 built and verified.**
- **T4 `EXP_TRANS_SKIP`** (`candidate/transliterate.c`): at setup the loaded trie is checked — html-escape's 65,788 rules all start with `&`,
  latin-ascii-simple step "1"'s 138 rules are each exactly one codepoint (`raw/t4_trans_rules_all.txt`, tool `tools/trans_rules_all.c`). At run time a valid
  UTF-8 string (decided with the engine's own `utf8proc_iterate`) without `&` and without any of the 138 codepoints is returned as a copy; without `&` only,
  the html-escape step is skipped. Invalid UTF-8 always takes the original path (the original crashes on it; pre-existing). Differential test
  `tools/t4_diff.c` (same binary, skip on vs off): 25,726 corpus lines + 3,000,000 random valid strings per seed, seeds 1 and 2: 0 mismatches
  (`raw/t4_diff_s*.txt`; a first run with stray invalid bytes hit the original library's crash and was restricted to valid UTF-8, as the project fuzzer does).
  End-to-end (`tools/verify.sh`): dumps, frozen references and 2 x 2 M fuzz identical (`raw/verify_t4/summary.txt`).
- **T8 `EXP_ALLOC_LEAN`** (`candidate/crf.c`): C3 aliases the context's transition matrix to the model's (restored before destroy; the other two scoring paths
  skip the now self-copy); C2 keeps `state_trans` zero by re-zeroing only the cells the previous address wrote (full memset after growth, after any address
  not recorded completely, or after a list allocation failure). End-to-end identical (`raw/verify_t8/summary.txt`). R1/R2 (arena, one-block response) not built.
- **T6 `EXP_PHRASE_PREFILTER`** (`candidate/trie_search.c`): per (trie, start node) first-byte and first-byte-pair tables built from the double array's first two
  levels (all existing nodes, leaf tails included); a token is skipped only in the fresh state, and only if its first byte (1-byte tokens) or first pair
  (longer tokens) cannot be walked, leaving the exact state the original ends in. Verify build `t6v` runs every search with and without the filter and aborts
  on any difference: golden-v1 38,589 and novel 38,589 searches, 0 mismatches; fuzz on `t6v` checked 5,365,332 + 5,364,231 searches, 0 mismatches
  (`raw/t6v_fuzz_summary.txt`); `t6` end-to-end identical (`raw/verify_t6/summary.txt`).
- **T5 `EXP_NORM_TABLE`** (`candidate/address_parser.c`, uses T4's checked tables, so variant `t45` = T4 + T5): a 65,536-entry table (512 KB) of each
  context-free BMP codepoint's normalized bytes, computed at parser setup by the library itself (`normalize_string_latin` on the one-codepoint string).
  Context-free = the codepoint, its lowercase and the first codepoint of that lowercase's canonical decomposition are starters that cannot be the second
  element of a composition, not ignorable/control/format/separator/unassigned/surrogate/private-use, not Hangul (algorithmic composition), not '&' or one of
  the 138 transliteration-rule codepoints; 42,532 codepoints qualify. A valid string whose every codepoint is printable ASCII or in the table is normalized by
  concatenation; anything else runs the library. First differential run aborted from a buffer overflow in the TEST TOOL (64-byte buffer, 90-byte strings),
  fixed and rerun: per seed (1, 2) all 25,726 corpus lines (25,501 take the fast path), every table codepoint next to every printable ASCII character in both
  orders, 10 M random table-codepoint pairs and 3 M random strings: 21,021,517 fast-path answers compared, 0 mismatches (`raw/t5_diff_s*.txt`).
  `tools/verify.sh t45` running. Found while designing: Hangul jamo report no composition index in utf8proc 1.3 yet compose algorithmically — excluded explicitly.

**16:50-17:20 — T7, T11, combinations, batch script.**
- **T7 `EXP_WORD_MEMO`** (F1 only): `word_or_phrase_at_index(..., long_context = false)` memoized per token for one address; plain-word results keep the
  normalized-token pointer (so 015's pointer-keyed vocabulary memo still applies), phrase results are copied into a per-entry buffer because the function
  writes them into scratch buffers the next call overwrites (the buffers are used nowhere else). F2 (phrase features once per phrase) NOT built.
- **Combinations:** `all` = k1 + T4 + T5 + T6 + T8; `all7` = `all` + T7. **T11:** `pgo` / `pgolto` = `all` compiled with `-fprofile-instr-use` (profile from
  `dump_outputs` over `golden-v1/development.txt` only; `artifacts/pgo_profile/hashes.txt`), `pgolto` also `-flto=thin` (`tools/build_pgo.sh`; the first
  build failed because the profile path had spaces, `raw/build_pgo.log`; fixed by copying the profile to `~/.cache/libpostal-016-pgo/`, same hash).
- **Stats builds** `s` (= k1) and `sall7` (= all7) + `tools/stage_bench.c` for T2.
- Verification status at 17:20: `t4`, `t45`, `t6`, `t8`, `all` fully identical (dumps, frozen references, 2 x 2 M fuzz); chain running for `t7`, `all7`,
  `pgo`, `pgolto`. A first attempt to queue that chain deadlocked (two waiters each matched the other's command line) and was killed and restarted.
- **Not built this round, with reasons:** T9 (C1 weight rows next to keys) and T10 (per-word records) target lookup/memory work, which 014 (memory floor
  0.25-1.16 µs) and 015 (+2% from removing string work on half the lookups) show is not where most time goes; R1/R2 (arenas, one-block response) and F2
  are larger rewrites. They stay on the list for after the batch shows what remains.
- **AC batch** `tools/run_batch.sh` (waits for the verification chain; only verified variants are timed; every run is gated on AC): A controlled session of
  baseline, `k0` and all verified variants on golden-v1 + novel; B floor subset; C T2 stage timing (`s`, `sall7` on golden-v1, floor, ASCII); D T3 paired
  `h0` vs `k1` (20 rounds). Analysis tools `tools/analyze_runs.py` (reproduces 015's floor table exactly), `tools/analyze_stages.py`.

## Results (AC batch `tools/run_batch.sh`, 2026-09-30 17:31-22:10 PDT; paused 17:50-19:20 by request and waited on battery ~20:49-20:52)

Session A (`research/tools/run_unattended.sh`, 5 processes x 18 warm passes per variant and dataset, rotating order; `raw/batch/A-summary.md`). Two runs touched
by sleep/battery were excluded and rerun on AC (`raw/batch/RERUNS.txt`; originals kept as `*.excluded`): `novel-t2-baseline` (one pass 5.0x its median,
battery in its post snapshot) and `v1-all-t2-all7` (one pass 2.1x). An earlier full-batch attempt (22 runs) and a quick session (4 runs) were stopped at the
user's request and are kept, unused, in `raw/batch_attempt1_stopped/`, `raw/quick_attempt_stopped/`, `raw/batch_restart_tmp/`.

| variant | golden-v1 median µs | vs original | vs `k0` | novel median µs | vs original | vs `k0` |
|---|---:|---:|---:|---:|---:|---:|
| original | 34.54 | 1.000x | | 34.77 | 1.000x | |
| `k0` (= 015 `k1`, control) | 12.73 | 2.712x | 1.000x | 12.71 | 2.735x | 1.000x |
| `t4` transliteration skip | 12.12 | 2.85x | 1.050x | 12.15 | 2.86x | 1.046x |
| `t45` T4 + normalization table | **11.45** | **3.016x** | **1.112x** | 11.68 | 2.976x | 1.088x |
| `t6` phrase prefilter | 12.57 | 2.75x | 1.013x | 12.66 | 2.75x | 1.004x |
| `t7` word memo | 12.82 | 2.69x | 0.993x | 12.89 | 2.70x | 0.986x |
| `t8` lean scoring buffers | 12.75 | 2.71x | 0.998x | 12.82 | 2.71x | 0.991x |
| `all` (T4+T5+T6+T8) | 11.64 | 2.967x | 1.094x | 11.69 | 2.975x | 1.087x |
| `all7` (`all` + T7) | 11.62 | 2.972x | 1.096x | 11.65 | 2.984x | 1.091x |
| `pgo` (`all` + PGO) | 11.30 | 3.06x | 1.127x | 11.28 | 3.08x | 1.127x |
| **`pgolto`** (`all` + PGO + thin LTO) | **10.59** | **3.262x** | **1.202x** | **10.63** | **3.271x** | **1.196x** |

All estimators (median, P10, CPU time, cold first pass) agree (`raw/batch/A-summary.md`). Floor subset (B, 5 processes each, `raw/batch/B-floor-summary.md`):
original 33.38 µs, `k0` 14.02 (2.381x), `t45` 11.23 (2.972x), `all` 11.32 (2.949x), `all7` 11.32 (2.948x), **`pgolto` 10.31 (3.237x)**.
Peak footprint (median of 10 processes): original 2,613,381,352; `k0` 2,538,391,736; `t45` 2,538,817,720; `all` 2,539,030,712; `pgolto` 2,538,948,792.
Setup: original 3.0 s, `pgolto` 9.1 s. T5's table adds 0.5 MB.

Stage timing (C, `stage_bench`, median of 3 runs, µs/address; `raw/batch/C-stages-summary.md`), `s` (= k1) vs `sall7` (= all7):
normalization floor **2.821 -> 0.156**, full set 1.234 -> 0.097, ASCII 0.050 -> 0.045; context fill (T6) 3.275 -> 3.245 (no real change); features (T7)
2.661 -> 2.665 (no change); T8 moved ~0.17 µs from the decoder setup (1.182 -> 0.996) into accumulation (1.013 -> 1.174) = no net gain. Remaining
full-set stages for `all7`: context fill 3.25, features 2.67, lookups 2.18, accumulation 1.17, Viterbi/rest 1.00, response 0.80, tokenize 0.54.

Paired confirmation of 015 (D, 20 alternating rounds, `raw/batch/D-paired-summary.md`): `k1` faster than `h0` in 19/20 rounds, median +2.47%.

## Interpretation

- Normalization was the floor's problem and is now essentially solved (2.8 -> 0.16 µs on non-ASCII addresses): the floor rises from 2.38x to 2.97x.
- T6, T7, T8 are exact but save nothing measurable: the work they skip was cheap (the trie walks mostly fail on the first, cached node; the memo's
  saved call was small; T8's per-cell bookkeeping costs what the skipped memset saved). Same lesson as 012/015: profile share != saving.
- Compiler PGO + thin LTO adds ~10% on top of `all` without any source change (trained on the development split only; verified byte-identical).
- Distance to 3.5x: golden-v1 needs 9.87 µs (now 10.59, -0.72 µs, 6.8%); floor needs 9.54 µs (now 10.31, -0.77 µs, 7.5%).

## Decision

Accepted: **`pgolto` is the new best candidate** (3.26x / 3.27x / floor 3.24x). T4 and T5 accepted; T11 (PGO + thin LTO) accepted as a build step.
T6, T7, T8: no measurable gain; kept in the source behind their switches, not carried forward on their own.
Next: (1) PGO + LTO of `t45` alone (drop the neutral switches) — not built or timed yet; (2) experiment 017 (load/build time for memory and runtime);
(3) the remaining big stages are context fill, feature building and lookups.
