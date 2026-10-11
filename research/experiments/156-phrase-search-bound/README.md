# Experiment 156 — context fill's irreducible phrase-search cost with model-derived hash sets (decisive test before a context-fill rewrite)

Status: **pre-registration, written before any run** (`raw/created.txt`). Results are appended below.

## Why

155 showed that the record design passes the necessary 8× budget only if context fill also shrinks. On golden-v1, context fill plus the feature
function's own work must fit in ≤ 1.42 µs; on OSM in ≤ 0.59 µs. cpi's context fill is 1.66 / 1.17 µs (155, `bench_ext` scale).

The user's standing rule (2026-10-05): proceed with any path whose output is byte-identical, whose memory is ≤ the original's, and that shows a
measured or provable speed gain.

## Method

**Replay.** Feed each address's normalized and admin tokens (O2) to three greedy longest-match searches:
- the address dictionary (`all|` keys) over the normalized tokens;
- component phrases over the admin tokens;
- postal codes over the admin tokens.

Each search uses an open-addressing hash set built from the **model's** phrase keys (143 `keydump` TSVs, hashes verified). It holds full keys
with their data, plus every token-boundary prefix flagged "continues", which mirrors the trie's `' '` transition. The search runs from each start,
extends while "continues" is set, keeps the last full match, then restarts at last+1 (match) or start+1 (no match). That is
`trie_search_tokens_from_index`'s state machine. The component validity filter (`is_valid_component_phrase`) is applied.

Not covered:
- **Excluded (kept as cpi does them):** token normalization ×2 and per-token prefix/suffix search.
- **Covered in time:** joining a span into a buffer, hashing, probing and comparing the full key.

## Pre-registered rules

1. **Exactness.**
   - Per token, the dictionary, component and postal memberships (start, len, data) must equal O2's on golden-v1, novel, OSM and fuzz-s1.
   - Any mismatch is counted and explained.
   - Timing is interpreted only if golden-v1 and OSM have 0 mismatches. A production design must be byte-identical.
2. **8× feasibility (necessary).** PS_new cold ≤ 1.42 µs on golden-v1 and ≤ 0.59 µs on OSM. Context fill's other parts and the feature function's work must still fit beside it, so exceeding these means **8× is infeasible for this design**.
3. **Gain potential.** PS_new cold must be clearly below cpi's whole context fill (1.66 / 1.17 µs).
   - This is necessary for replacing phrase search to pay at all.
   - cpi's phrase-search share of context fill was never measured separately, so this gate is necessary, not sufficient.
4. **Memory.** The hash sets must not exceed the tries they replace: the component-phrase trie, the postal-code trie and the dictionary trie (load-time sizes, measured or from file sizes, labelled).
5. **Cross-session caveat.** Timing is compared with 155's same-day medians from another session. This is indicative only; a build candidate is re-measured in one session.

## Amendment before timing (2026-10-05, after the first exactness check; no timing had been run)

**Two exactness findings.**
1. The dictionary search with `lang = NULL` starts at the trie ROOT (`address_dictionary.c:241`), not at `all|`. Attempt 1 (`raw/attempt1_all_prefix/`) is kept; fixed by using all keys.
2. `trie_search_tokens_from_index` has a structure-dependent quirk, traced with the verbatim copy in `tools/trace_search.c`:
   - When a span ends at an explicit trie node with neither a `' '` continuation nor a match ("No continuation … yielding tokens"), the search skips past the span **without** retrying from start+1. Example: golden-v1 address 146, "/ a": "a" alone is never tried.
   - Falling off the trie mid-token does retry from start+1.
   - Matches inside tail-compressed suffixes also use token-length `strncmp`.

   Exact emulation therefore needs model-derived flags that describe the trie's explicit nodes and tail boundaries.

**Amended reading (declared now, before any timing).** Rule 1 (exactness) is not met by this replay, so its timing is **an optimistic lower bound** on an exact hash design: emulation adds checks and fallbacks, it never removes probes.
- If the bound **fails** rule 2 (8×), that verdict stands for any exact version.
- If it **passes**, nothing is concluded until an exact emulation is built and re-timed.
- Rule 3 (gain potential) is read the same way.

## Results (`raw/exact/`, `raw/timing/`; 3 rounds × 60 passes; 6/6 exit 0; 0 gate failures)

### Exactness after the root fix (rule 1 is NOT met; the remaining mismatches are the trie-structure quirks)

| corpus | addresses with any membership mismatch | token mismatches: dictionary / component / postal |
|---|---:|---|
| golden-v1 | 61 / 12,863 (0.47%) | 45 / 41 / 4 |
| OSM v3 | 28 / 11,405 (0.25%) | 19 / 19 / 0 |
| novel | 48 / 12,863 (0.37%) | 26 / 38 / 1 |
| fuzz-s1 | 1,738 / 178,919 (0.97%) | 871 / 2,311 / 205 |

### Timing: optimistic lower bound of an exact hash-based phrase search (µs/address)

| | golden-v1 cold (r1 / r2 / r3) | warm | OSM cold | warm |
|---|---|---|---|---|
| PS_new | 0.975 / 0.779 / 0.805 → **0.805** | 0.32 | 0.422 / 0.421 / 0.418 → **0.421** | 0.27 |

**Measurement shortfall.** Each process ran mostly set-building. Measured search time per variant is about 1–2 s, below METHOD's 10 s.

### Reading under the amendment

- **Rule 2 (8× necessary).** The bound passes: 0.805 ≤ 1.42 on golden-v1, 0.421 ≤ 0.59 on OSM. As declared, nothing is concluded for an exact design. On OSM only ≈0.17 µs would remain for normalizing every token twice, the prefix/suffix search and all of the feature function's own work. **8× on OSM is therefore implausible even though the bound passes.**
- **Rule 3 (gain potential).** The bound is 48% (golden-v1) and 36% (OSM) of cpi's whole context fill (1.66 / 1.17 µs). So a hash-based phrase search has room to cut context fill. cpi's phrase-search share was never measured separately, so how much of that room is real is not known.
- **Rule 4 (memory). FAIL as built.**
  - The sets take 946 MB: dictionary 26.7, components 474.8, postal 444.4. The tries they replace are ≈768 MB as on-disk files: `address_parser_phrases.dat` 137.5 MB and `address_parser_postal_codes.dat` 621.6 MB (both including non-trie data), plus the dictionary trie.
  - A compact table would fit: 12-byte entries at load 0.75 come to ≈164 MB of tables + 115 MB of keys ≈ 0.3 GB. That is a derived figure, UNVERIFIED.

### Combined optimistic projection for the full exact redesign (derived; UNVERIFIED)

The total is ntr + CF_new + B_new + feature-function work, with X = normalization + prefix/suffix (not measured) and Y = feature-function control flow (not measured).
- **golden-v1:** 0.442 + (0.805 + X) + 2.111 + Y. With X + Y = 0.3–0.8 µs, that is **≈7.6–8.2×**.
- **OSM:** 0.364 + (0.421 + X) + 1.509 + Y, giving **≈6.4–7.6×**.
- **Against cpi (155):** 6.06× / 5.11×, so roughly **+25% to +45% potential**. It is backed by measured components, but exactness and memory each need real engineering.

## Decision

- **8× is not reachable on OSM with this design.**
- **A measurable gain over cpi is plausible**, so the user's standing rule (byte-identical, memory ≤ original, data-backed gain) is met and work continues. Order:
  1. build the record-based feature path into cpi, keeping cpi's context fill. This is 155's +≈25% potential and a prerequisite for the full design;
  2. separately, emulate the trie quirks exactly with model-derived flags and compact tables (memory rule 4), checked against the oracles and fuzz.

Every step stays gated on byte-identical output and on peak memory ≤ original − 20 MB.
