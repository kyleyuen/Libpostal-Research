# Experiment 146 — REWRITE_PLAN M2: H-build of unit ids + integer bigram ids, on the ORIGINAL sources, against the 142 oracles

Date/time/timezone: created 2026-10-04T21:14:13-0700 (`raw/created.txt`), America/Los_Angeles. Commands: `raw/commands.txt`.
Status: complete — **M2 passed** (exact on every oracle, corpus and fuzz input, first attempt)

## Hypothesis and kill rule (written before coding)

User approval 2026-10-04: "a gain is a gain lets do it" (after `research/analysis/REWRITE_MATH_2026-10-04.md`).

**Design (exact by construction).**
- **Units.** U = vocabulary words with count > 0 ∪ component-phrase keys ∪ address-dictionary phrases (the text after `lang|`) ∪ {`UNKNOWN`, `UNKNOWN_NUMERIC`}. Each unit gets an interned id at load time.
- **Pair table.** At load, every `prev word+word` / `word+next word` key of the model is split at every `|` into (A, B). Every split whose two parts are both units is inserted as (family, uid(A), uid(B)) → feature id.
- **Why the lookup is exact.**
  - Take any runtime bigram whose parts A and B are both units, where A is non-empty and does not end in `|`. `feature_array_add` would build exactly "fam|A|B", because its separator stripping does not apply.
  - That key exists in the model if and only if (fam, uid(A), uid(B)) is in the table. Every split of it, including (A, B), was inserted.
  - So the id or the miss is the same as the original lookup.
  - Every other runtime bigram keeps the original string path.
- **Emission order is preserved.** A placeholder string `"\x01"` is pushed at the same position, and its id (or MISS) goes on a per-token side channel. `crf_tagger_score` resolves the placeholder in place, so the ordered id sequence, and with it the accumulation order, is unchanged.
- **Build line.** H-build = original sources + `EXP_ORACLE` + `EXP_RW_BIGRAM`. Correctness only; never timed.

**Pass condition.**
- O1, O2, O3i and O4 are byte-identical to `research/oracles/v1` on golden-v1, novel, OSM and fuzz-s1-200k. O3k is expected to differ at the placeholder positions only.
- `dump_outputs` is identical on all sets.
- The full 2 × 2M fuzz is identical.
- Counters report what share of bigrams took the integer path.

**Kill rule (plan M2).** If the build is not exact after 3 sessions, drop the bigram family and report why.

## Reproduction

- **Sources.** 142's oracle-instrumented original `address_parser.c` and `crf.c`, plus the new `src/rw_bigram.h`. Its trie enumerator is copied verbatim from cpi's `feature_dict.h`. Diffs against 142 are in `raw/*.vs142.diff`; all changes are under `#ifdef EXP_RW_BIGRAM`.
- **Builds** (`tools/build.sh`, original flags): `c146` with no flags (control) and `hrw` with `-DEXP_ORACLE -DEXP_RW_BIGRAM`. Both exit 0, and the warning count is unchanged.
- **Verification.** `tools/verify.sh`, 2026-10-04T21:16:54 → 21:19:02 (`raw/v/run.log`); 8 runs, all exit 0. Correctness only, never timed. Summary in `raw/verify_summary.txt`.

## Results

- **Load** (`RW_VERBOSE`). 7,345,126 units (equal to 143's count) and 5,011,105 pair entries (= 2,884,139 + 2,126,966 unique model splits; no key splits two ways). It takes 4.6 s on the H-build.
- **Oracles.** O1, O2, O3i and O4 are **byte-identical** to `research/oracles/v1` on golden-v1, novel, OSM and fuzz-s1-200k. O3k differs on exactly as many lines as there are placeholders: 123,002 / 120,300 / 96,681 / 2,344,780.
- **Outputs.**
  - golden-v1 sha `45574ee2…` (hrw and c146), plus a match against the frozen `baseline-outputs.jsonl` (IDENTICAL);
  - novel `806b8a81…`;
  - OSM identical;
  - unseen v11 identical to the original's dump (134);
  - fuzz seeds 1 and 2 (2 × 2,000,000) identical.
- **Path coverage (runtime counters).**
  - golden-v1: 123,002 bigrams via integers, 6 via the string fallback;
  - OSM and v11: 100% via integers;
  - fuzz: ≈35k per seed via the string fallback, including 233–266 left parts ending in `|`.
  Both paths were exercised.

## Decision

M2 passed. The exactness of the integer bigram design is established on the original code.

Next step (M3): the R-build on cpi. cpi already contains 122's vocabulary-canonical pair path (`EXP_INT_KEYS_PAIRS`). Experiment 147 first measures where that path's canonicalization cost goes, which is the open term of the cost model.
