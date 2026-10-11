# Redesign plan — compiled records + exact hash phrase search (NEXT STEP; nothing built or run)

Status: **planned 2026-10-05 at the user's request ("let that be the next step"). Not started.**

Every number below comes from a cited experiment. Every speed figure for the redesign is a projection (UNVERIFIED) until a phase measures it.
`cpi` (`final/119-cpi`, 121) remains the deliverable until P4 passes.

## Rules (unchanged, all hard)

| Requirement | Limit or rule |
|---|---|
| Output | Byte-identical on golden-v1 and every other set |
| Threads | One parsing thread |
| Memory | Peak ≤ original 2.614 GB and steady ≤ original; working kill line peak > original − 20 MB |
| Tables | Derived from the model only |
| Caching | No caches across addresses |
| Baseline | The preserved original (000-baseline) |
| Validation | Reserved v12 used once, at P4 |

**User's go rule (2026-10-05).** Proceed on any step that keeps the output byte-identical, keeps memory ≤ original, and shows a measured or provable gain.

## Why this design (evidence)

| Fact | Source |
|---|---|
| cpi is within ≈2.4–4.6× of its own architecture's floor, so tuning is exhausted | 151 |
| Adding rows in emission order is bit-exact with any layout, SIMD across labels and vectorized Viterbi; no certificate needed | 152 PROOF §6 |
| Compiled records (unit / pair / own) hold every model row bit-exactly; 1,156 MB as timed vs 1,171 MB of cpi structures replaced | 153, 155 |
| Block after context fill: records 2.111 vs cpi 3.155 µs (golden-v1), 1.509 vs 2.308 (OSM), labels identical | 155 |
| Feature-function work that stays (control flow, joins, vocabulary probes) Y = 0.92 / 0.60 µs | 157 |
| Hash phrase search bound 0.81 / 0.42 µs vs cpi's whole context fill 1.66 / 1.17 µs; not yet exact (trie quirks) | 156 |

**Projection (UNVERIFIED).**

| | Records inside cpi only | Full redesign |
|---|---|---|
| golden-v1 | ≈ +4% | ≈7.6–8.2× (cpi 6.06×) |
| OSM | ≈ +8% | ≈6.4–7.6× (cpi 5.11×) |
| Source | 157 | 156 |

8× on OSM is not reachable with this design (156).

## Phases, each with a pre-registered kill rule

Each phase is a numbered experiment. The kill rule is written before the phase runs, and failed attempts are kept.

### R0 — repair (independent, small)

Fix cpi's `libpostal_parser_print_features` abort (`final/119-cpi/sources/crf.c:793`). Add `libpostal_setup_parser_datadir` to the API probe.

**Gate.** Outputs identical on all sets, timing unchanged within noise, and both APIs behave like the original.

### P1 — exact hash phrase search (offline from the model)

**Goal.** Reproduce `trie_search_tokens_from_index` exactly. 156 traced two structure-dependent behaviours:
- "no continuation → skip without retry at start+1";
- token-length `strncmp` inside tail-compressed suffixes.

**Method.**
- Emulate them with flags derived **from the tries at build time**: explicit-node spans, tail boundaries and continuation flags per stored span.
- Use a compact open-addressing table (12-byte entries, load ≈0.75).
- Prefix/suffix search stays as in cpi (`EXP_PS_FLAGS`).

**Checks.**
- Per-token dictionary, component and postal memberships equal O2 on golden-v1, novel, OSM and fuzz-s1.
- New v2 oracles for 2 more fuzz seeds × 2M (`research/oracles/v2`; v1 untouched).
- Timing in one session against cpi's context fill.

**Kill.** Any mismatch that cannot be explained, the tables exceeding the tries they replace (≈768 MB on disk), or no measured context-fill gain.

### P2 — records inside cpi (feature path)

**Goal.** Ship the record arena as an **offline file derived from the model**, memory-mapped, so load time stays ≤ cpi's 11.5 s.
- It replaces cpi's feature dictionaries, packed rows, `indptr` and vocabulary dictionary.
- The feature function emits (record, slot) in original emission order. Neighbour words reuse records within the address, and the vocabulary frequency is read from the record.

**Checks.**
- The per-token hit-id sequence equals O3i on 4 corpora.
- Outputs are identical on all 13 non-reserved sets, the 150 holdout and 2×2M fuzz.
- Peak and steady memory are measured; one thread.

**Kill.**
- Any output difference.
- Peak > original − 20 MB.
- Same-session gain < +3% against cpi on golden-v1 and OSM. 157 projects +4–8%.

### P3 — context fill on P1

**Goal.** Replace the three phrase tries with P1's tables. Keep normalization and prefix/suffix.

**Checks.** Same exactness set as P2, plus O2 equality.

**Kill.** Any output difference, memory over the limit, or a whole-parser gain < +10% against cpi in one session.

### P4 — integration and validation (METHOD)

**Checks.**
- All sets and fuzz byte-identical.
- API probe passes, including feature-print and setup_datadir.
- Peak and steady memory, threads and load time measured.
- 5 rounds × 20 passes against the preserved original.
- The reserved v12 set used once.

**Then.** Only then may the professor report change.

## Risks (honest)

- **149's precedent.** Integer ids inside the current design gained 0%, and P2's estimate is only +4–8%.
- **Memory.** About 15 MB of headroom as timed. The fallback is a smaller index (≈ −134 MB, derived).
- **The P1 quirk emulation is intricate.** It is fully checkable against the oracles and fuzz, but it is not yet designed in detail.
- **Measurements to redo.** All timing must be repeated in single sessions. 155–157 mix sessions in places and are labelled where they do.

## Alternatives that need a rule change (for discussion with the professor, not planned)

| Change | Rule broken | Rough effect |
|---|---|---|
| Parse on several threads | single thread | up to ≈8× throughput on 8 P-cores (UNVERIFIED scaling) |
| Smaller or retrained model | same model | fewer features and rows; accuracy must be re-scored |
| Approximate output (pruned features, float32 without fallback) | byte-identical output | 145: deleting one family changed 6.6% of parses for 7–9% speed |
