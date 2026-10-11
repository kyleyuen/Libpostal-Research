# Audit plan — "compiled lexicon + certified argmax" architecture (from experiment 151)

Status (updated 2026-10-05, after experiments 152–155):
- **A1 passed** (152).
- **A2 passed** (153).
- **A3 is done** (154): exact back half ≈0.83 µs warm, projection lowered.
- **A4 halted at its decision point** (155): necessary condition PASS, hybrid FAIL. Continuing needs a context-fill rewrite, which awaits the user's decision.
- A5 and R0 have not started.
- **Next step (agreed 2026-10-05, not started):** superseded by `research/plans/REDESIGN_PLAN_2026-10-05.md`. It covers phases R0 and P1–P4, which fold in A4's open context-fill work and A5.

Original status: **plan only; nothing built.** Each milestone becomes its own numbered experiment (152+) recorded with the TEMPLATE, and every failed
attempt is kept. Every speed figure here is a target or an UNVERIFIED projection until a milestone measures it.

## Constraints (unchanged)

| Requirement | Limit or rule |
|---|---|
| Output | Byte-identical default-parser output on every golden-v1 input. Also checked on novel, OSM v3–v11, the 150 holdout, 2×2M fuzz and the 142 oracles at every stage. |
| Threads | One parsing thread. |
| Memory | Peak ≤ original 2.614 GB and steady ≤ original. The kill line is **peak > original − 20 MB**, as in REWRITE_PLAN. |
| Model and decoding | Same model file, same CRF and Viterbi semantics. No memoization across addresses. |
| Tables | Every table is derived from the model, never from benchmark data. |
| Baseline | Comparisons use the preserved original executable/library (000-baseline), per METHOD. |
| Validation | Reserved v12 is used once, at A5. |

Not allowed, because each would need a rule change by the user and professor: threads, cross-address caches, a smaller or retrained model, and approximate output.

Already measured and rejected, so not re-proposed:
- integer keys inside the current design (146–149);
- interleaving addresses (139/140);
- the memory micro-experiments listed in handoff §6.

## Why this architecture (151 evidence)

- **Float order is not pinned.** A certified argmax with an exact fallback keeps byte-identity: certified on 100% of golden-v1, novel and OSM addresses, and 99.94–99.99% of fuzz.
- **The design needs fewer random accesses.** Compiled records need 16.6 random accesses per address against 101.5 key probes and rows today (golden-v1), before counting trie walks. They touch 81 distinct lines against 152.
- **`cpi` spends most of its instructions on non-model work.** It executes ≈50k instructions per address; the model's arithmetic is ≈7k scalar operations. `cpi` sits ≈5–13× above the new design's floor (≈0.43–1.1 µs), while the current architecture's own floor is ≈1.2–2.3 µs (151, same method).

## Milestones and kill rules

Each milestone's kill rule is declared before it runs. All timing follows METHOD: environment gate, AC power, serial runs, ≥10 s per trial, preserved baseline.

### A0 — feasibility of exact restructuring: DONE (151)

Passed. Rows were rebuilt bit-exactly, and 100% of real addresses certified.

### A1 — certificate proof and fuzz at scale

**Goal.**
- Write the certificate as a short proof for the professor: the per-entry error bound, interval propagation through the monotone DP, strict separation, and tie handling.
- Implement it cheaply: per-row Σ|w| comes from the model, and gaps are checked only on the decoded path.

**Test.**
- Generate 142-style oracles for at least 2 more fuzz seeds (2M inputs each, correctness only) into `research/oracles/v2`. v1 is never touched.
- Run `oracle_bound` and a K2 variant with the production certificate on every corpus.

**Kill rule.** Stop if any of these happens:
- any label mismatch after fallback;
- fallback above 0.1% on a real corpus, or above 1% on fuzz;
- the proof needs an assumption that cannot be checked at runtime.

### A2 — memory-feasible record format

**Goal.** Choose the value precision so that peak and steady memory stay within the limits.
- **Default: f64 records with free summation order (E1).** About 510 MB, replacing the 620 MB CSR arrays. Fallback replays the original order from the same values, so no second copy is needed.
- **f32 records plus f64 fallback values:** only if a measured layout fits.

**Test.** Build the arena offline from the whole model, with no data input. Every model key must map to exactly one (record, slot), and the row read back must be bit-identical in values and labels.

**Kill rule.** Stop if any of these holds:
- any key is unmapped or reconstructs differently;
- the arena plus its index exceed the arrays they replace;
- the projected peak is above original − 20 MB.

### A3 — back-half engineering (bound, not candidate)

**Goal.** Bring the replay kernel's K2 (1.77 µs cold / 0.75 µs warm on golden-v1) toward the ≈0.36 µs compute floor:
- dense SIMD adds for 20-nonzero rows, which are most state hits;
- no per-token L×L zeroing;
- vectorized Viterbi.

**Kill rule.** None; A3 is diagnostic. If warm K2 stays above 0.6 µs on golden-v1, lower the projected ceiling to match.

### A4 — front half without string keys (the main risk)

**Goal.** Tokens and context must produce **exactly the O3i feature-id sequence per token**: the same ids, and the original order wherever fallback replays it. The route:
- one record probe per word, word pair and phrase;
- position flags (first word, and similar);
- postcode context and phrase features.

**Approach.**
1. Hybrid first: keep libpostal's normalization, tokenizer and phrase search, and replace only key building and probing. Check against the O2/O3 oracles at every stage.
2. Then replace the phrase-search tries with record-driven span probes.

**Kill rules.**
- **Exactness.** Any O3i or O4 mismatch on any corpus that can't be fixed stops the track.
- **Speed.** The warm front half must be **≤ 50% of `cpi`'s**, i.e. ≤ 0.63 µs golden-v1 and ≤ 0.54 µs OSM (141 split harness: 1.26 / 1.08). Otherwise the realistic projection falls below ≈8×, and the track stops.
- **Cold speed.** In the `bench_ext` cold pattern on the split harness, the front half must be **≤ 1.3 µs golden-v1 and ≤ 1.2 µs OSM**. These are the upper ends of the projection's front-half and miss terms. `cpi` today is 3.77 / 2.28 µs (141). Most of `cpi`'s gap is cold stall, so passing only the warm rule is not enough.

### A5 — integrated candidate

**Goal.** A full parser with load and API lifecycle, timed against the preserved original.

**Requirements.**
- Byte-identical on all sets, 2×2M fuzz and the 150 holdout.
- One thread.
- Peak and steady memory measured.
- Load time reported; it must not exceed `cpi`'s 11.5 s, which means the lexicon is compiled offline from the model.
- `libpostal_parser_print_features` works.

**Accept rule.** The candidate must beat `cpi` on the 121/150 measurement sets by more than the noise floor. It is then validated once on v12. Only then may the professor report change.

## Independent repair (any time)

**R0.** Fix the `cpi` feature-print abort (`research/final/119-cpi/sources/crf.c:793`, `EXP_ROW_PACK`). Re-run 150's API probe and the full correctness set; timing must be unchanged within noise.

## Order

1. **A1, A2 and R0 first.** They are cheap and decide feasibility without touching the parser.
2. **A3 can run in parallel.**
3. **A4 decides whether the projected ≈10–14× (UNVERIFIED) is real.** It is the largest milestone and carries the main risk.
4. **A5 only if A4 passes.**
