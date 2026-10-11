# Kill rule — pre-registered 2026-10-05T17:16:01-0700, before any build (copied verbatim from plans/REDESIGN_PLAN_2026-10-05.md, phase P2)

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

