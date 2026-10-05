# Experiment 143 — integer-pipeline probe: model key families, unit-id coverage, table memory (REWRITE_PLAN M1)

Date/time/timezone: created 2026-10-04T19:31:26-0700 (`raw/created.txt`), America/Los_Angeles. Commands: `raw/commands.txt`.
Status: complete — rules 2 and 3 pass; rule 1 deferred to a measured peak check (see Results)

## Hypothesis and kill rules (copied from the plan before running; not changed afterwards)

Approved by the user 2026-10-04 ("now run the M0 + M1"). Plan: `research/plans/REWRITE_PLAN_2026-10-04.md` §5 M1, cards C1/C3.

**Question.** Can the remaining string families, above all the bigrams, be produced as integer ids from tables built from the model alone, within the memory headroom?

**Unit.** A unit is the canonical string a word-family key carries:
- a vocabulary word with count > 0;
- a component-phrase string;
- an address-dictionary phrase string;
- `UNKNOWN` or `UNKNOWN_NUMERIC`.

**Kill rules (from plan M1).** Stop the track if any of these holds:
1. the extra tables need > 50 MB of peak memory;
2. unit coverage of runtime bigram parts is < 90%, measured on golden-v1 development and OSM addresses via the 142 oracles;
3. > 5% of the model's bigram keys cannot be split unambiguously into two parts that are both units.

**Method (no parse-path change).**
- `tools/keydump.c`, linked against the 142 `ctl` original library, enumerates every key of the model's tries with cpi's verified enumerator (`fd_for_each_key`, as in 006/118). The tries are: state features, state-transition features, vocabulary, component phrases, and address dictionary.
- `tools/analyze.py` then:
  - splits the model's word-family keys into units;
  - sizes the tables from model counts only;
  - measures runtime coverage from the 142 O3k/O3i oracles.
- **Coverage is measured on data. Table contents and sizes are derived from the model only (§2.5).**

**Declared limitation.** The plan also asked for the *time* cost of computing unit ids with stage counters. That needs a parse-path change, which M1 excludes. M1 therefore reports the work count instead: unit lookups per token and how many are already done today. The timing moves to the first H/R build.

## Reproduction

- `tools/keydump.c`: the enumerator is copied verbatim from `final/119-cpi/sources/feature_dict.h`. It is linked against 142's `ctl` original `libpostal.a`.
  - Build attempt 1 failed (`LIBPOSTAL_DATA_DIR` not defined; `raw/attempt1_build.*`); attempt 2 exit 0.
  - Run exit 0 (`raw/keydump.stdout`). Every trie enumerated completely, with key counts equal to the header counts and 0 skipped: state 11,470,623; transition 2,086,474; vocabulary 3,694,560; component 4,012,750; postcode 5,483,892; dictionary 153,670.
  - Dumps are in the session scratchpad (1.4 GB, regenerable); hashes in `raw/keys.sha256`.
- `tools/analyze.py` → `raw/analyze.stdout` (exit 0, 12.8 s, 1.05 GB RSS). Runtime inputs: `research/oracles/v1` golden-v1 (10,308 development addresses mapped by string) and OSM v3.
- cpi load-time dictionary sizes: `CRF_DICT_VERBOSE=1` on 119's `cpi` `dump_outputs` (`raw/cpi_dict_verbose.stderr`; load only, not timed).

## Results

**Kill rule 3 (split ambiguity): PASS.** Model keys that do not split into exactly one (unit, unit):
- `word+next word` 0.23% (6,586 of 2,890,725);
- `prev word+word` 0.35% (7,556 of 2,134,522);
- unigram families 0–0.46%.

No key splits two ways (2+-split = 0 everywhere). The 0-split keys stay on the string path. The exception is `sub_word` (69.9% not unit-splittable); it fires 0.02–0.07 times per address and stays on strings.

**Kill rule 2 (runtime coverage): PASS.** Share of emitted word-family keys whose parts are units:
- golden-v1 development: 99.8% overall (31.2 keys per address); bigrams 100.0%;
- OSM: 100.0% (27.9 keys per address); bigrams 100.0%.

Only 43–47% (dev) and 61–65% (OSM) of emitted bigram keys exist in the model. Over half of the bigram probes are misses today, and a miss must stay cheap in the integer table.

**Kill rule 1 (> 50 MB extra peak): not triggered on the available evidence, but not yet measured for this design.**
- **Gross.** New id-only tables sized from model counts (open addressing, load 0.75) come to 142.6 MB:
  - bigrams 80.1 MB;
  - unigram state families 23.4 MB;
  - transition families 22.2 MB;
  - component/dictionary → unit maps 16.7 MB.
  This would apply only if they were added *on top of* the existing dictionaries.
- **The design replaces entries instead.** cpi already holds the unigram word families as integer keys (015 `k1`: 1,642,049 state + 1,424,243 transition keys moved; `raw/cpi_dict_verbose.stderr`). The remaining string state dictionary is 398.2 MB for 9,828,574 keys (≈40.5 B/key including packed rows).
- **What is new is mainly the bigrams.** Experiment 122 moved the bigram keys (vocabulary-canonical pairs) of cpi into the integer table and measured peak **−48 MB** (2,495,414,272 vs 2,543,124,480 B).
- **Net estimate (UNVERIFIED for this design).** ≈ −48 MB + 16.7 MB unit maps, i.e. a net decrease. Phrase-unit pairs add entries beyond 122's vocabulary-only pairs.
- **Consequence.** The binding check moves to the first built candidate: M2/M3 must measure peak and steady footprint. A new declared kill rule: **stop if R1's peak is > original − 20 MB** (i.e. > 2,594 MB).

**Work count, instead of the time the plan asked for.** In the original code every token's word is already looked up in the vocabulary (`word_vocab_frequency`), and the prev/next words are looked up again for each neighbour. A phrase token's component or dictionary match already carries its data id. So computing one unit id per token needs no new hash or trie lookup: only a data-id → unit array read for phrase tokens. Time is measured only once built (M2/M3).

## Decision

- **M1: kill rules 2 and 3 pass.** Rule 1 is not triggered on measured evidence (122) but is unconfirmed for this design. It is re-declared as a peak check at the first build.
- The track may continue to M2 only with the user's approval (the user approved M0 + M1 only).
- **Not done:** the plan's "fetch v13 during M1". It is a 1–6 h Overpass network job, so it awaits the user's go-ahead.
