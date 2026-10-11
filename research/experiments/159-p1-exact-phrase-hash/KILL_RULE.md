# Kill rule — pre-registered 2026-10-05T17:16:01-0700, before any build (copied verbatim from plans/REDESIGN_PLAN_2026-10-05.md, phase P1)

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

