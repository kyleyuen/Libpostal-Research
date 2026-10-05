# Experiment 030 — remaining string-keyed features: count, pair-routing screen, phrase-key feasibility

Date/time/timezone: record created 2026-10-02T14:07:42-0700 (`raw/created.txt`), America/Los_Angeles.
Status: screen complete; phrase-key build not started (decision pending with the user)

## Hypothesis and change

Measured bottleneck: 029 shows feature generation 2.03 µs + lookups 1.15 µs of `l4`'s ~8.0 µs. Features are built as text (`name|value`) and the lookup splits, hashes and compares them to recover an integer id. Experiment 015 moved single-word features to integer keys; the rest still use strings. Hypothesis: routing more feature names through model-derived integer keys removes instructions in both stages.

**Generality rule (user instruction 2026-10-02):** no tuning to our corpora. Any table is derived from the model files only and routed by feature name for every name of a family, never by which names or values were frequent in our data. Development counts below only set ceilings. Note that every `l4`-procedure build, including this experiment's, trains PGO on golden-v1 development addresses (inherited from 021; disclosed in the professor report as a limitation).

Base: unchanged 021 `l4` sources, copied to `sources/` (`raw/base_sources.sha256`). Build script `tools/build_cc.sh` = 021's with E=030 and sources from `sources/` (or `$SRCDIR`).

## Step 1 — where string lookups remain (measurement only)

Command: `/bin/bash tools/count.sh` (14:07:58–14:08:15). Build `cnt` = `l4` switches + `EXP_DUP_STATS`, no PGO/LTO (counts only); `dump_outputs` on `profiling/golden-v1/development.txt` with `DUP_NAMES=1`. Build and dump exit 0 (`raw/count_build.exit`, `raw/count_dump.exit`). The END line of `raw/run.log` printed empty exit codes because of an unquoted path in `count.sh`; the actual codes were appended as a NOTE line.
Totals (`raw/count_names.stderr`): 562,743 lookups = 54.6/address (matches 029's 45.86 + 8.74); **string path 323,209 (57.4%)**, integer/resolved 239,534; 35,138 string keys repeat an earlier key of the same address. By name (`data/string_names.txt`, per address): phrase type+phrase 6.06, prev word+word 4.80, word+next word 4.76, phrase 3.11, word 2.20, commonly city 2.18, unambiguous phrase type+phrase 1.95, unambiguous phrase type 1.95, postcode no context 1.06, prev word 0.76, postcode have context (with value) 0.71, next word 0.50, other commonly * 0.93, suffix 0.25, others < 0.1.
Families: phrase-valued (phrase, phrase type+phrase, unambiguous phrase type(+phrase), commonly *) ≈ 16.2/address (30% of all lookups); word pairs 9.6 (17%), not routed in `l4` because `EXP_INT_KEYS_PAIRS` is off.

## Step 2 — screen: existing pair routing (`EXP_INT_KEYS_PAIRS`) vs `l4`

Prior evidence: 015 `k2` (pairs) +0% / −0.8%; 020 `g2` vs `g1` 0.991× / 0.989× / 1.005× (golden-v1 / novel / OSM). The same canonicalize-and-probe mechanism would carry phrase strings, so this cheap screen decides whether the string→integer family still has room. Rule set before running: < ~1% fewer instructions/address → stop this family.
Command: `caffeinate -i /bin/bash tools/screen_pairs.sh` (build `pr` with the `l4` procedure + `-DEXP_INT_KEYS_PAIRS`; output check vs `l4` on development; instructions/cycles by difference of 2 vs 102 passes, 3 rotating reps). Raw: `raw/screen_pairs/`.

Results (`tools/analyze_screen.py` → `raw/screen_pairs/analyze.stdout`; run 14:12:54–DONE marker; all steps exit 0, AC gate passed each run, host load average ~7–8 in `raw/screen_pairs/env.txt`):
- Output of `pr` byte-identical to `l4` on all 10,308 development addresses (`raw/screen_pairs/output_check.txt`).
- Instructions/address: `l4` 72,616 / 72,763 / 72,968 (median 72,763); `pr` 71,722 / 71,302 / 72,240 (median 71,722) → **0.9857× (−1.4%)**, ≈108 instructions saved per routed pair lookup (9.6/address).
- Cycles/address: `l4` median 31,889 (31,206–33,074), `pr` 30,645 → 0.961×, within `l4`'s own rep spread.
- Warm-pass median µs/address in the same runs: `l4` 7.75–8.24, `pr` 7.68–7.88 → 0.997× (no wall-time gain; not a controlled timing).
Interpretation: just above the pre-set 1% screen line but not a clear drop, and consistent with 015/020 (pairs gave no measurable time gain). Extrapolating the per-lookup saving to the 16.2 phrase lookups/address gives roughly 2–3% of instructions for string-routed phrases (UNVERIFIED estimate); a phrase-id design that also skips building the phrase string could save more, but needs the exactness proof below.

## Phrase-ID feasibility notes (code reading only, upstream `libpostal/src/trie_search.c` 240–400)

A key by trie phrase id would avoid building the phrase string. Exactness requires that the joined phrase (`cstring_array_get_phrase`: tokens joined by one space) always equals the trie key for that id. The token search skips WHITESPACE tokens, matches tails with `strncmp` over the token remainder and continues across tokens, so equality is plausible but not proven; a verify build would be required. Not pursued unless step 2 shows room.

## Decision

Screen: marginal (−1.4% instructions, no wall-time change). Pair routing is not adopted on this evidence. Whether to build the phrase-id candidate was put to the user with the expected size and the proof cost.
