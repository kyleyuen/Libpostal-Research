# Experiment 056 — exact first-token table for the component-phrase trie (+ early role-row prefetch), on 053 `pc`

Date/time/timezone: created 2026-10-02T18:42:12-0700; screens 2026-10-02T18:44:40-0700 → 2026-10-02T18:49:24-0700, America/Los_Angeles.
Status: **accepted into the stack** (`ftr`); full validation pending with later changes

## Change
- `EXP_FTT` (`trie_search.c`, `address_parser.c`): at load, every vocabulary key (3,694,560) is walked through the component-phrase trie with the token search's byte-loop rules; the table (56.4 MB, indexed by vocabulary slot) stores the step count k, the node before the last step and the last node. Context fill looks up each admin token's bytes in the vocabulary (−1 if absent) and, for a fresh token from the root, the search starts its byte loop at step k with those nodes; all branches after that are the original code. Model data only. `EXP_FTT_VERIFY` recomputes every table-assisted walk byte by byte and aborts on a difference.
- `EXP_FTT_ROLE_PREFETCH`: in the same loop, prefetch the role-id row of each token found in the vocabulary (feature generation reads it later). Prefetch only.

## Checks
Verify build: 43,274 (golden-v1) + 41,775 (novel) + 35,571 (OSM) + 7,165,877 (2M fuzz, seed 1) table-assisted tokens checked, **0 mismatches**; outputs identical to the original (golden-v1, novel shas; OSM; fuzz) (`raw/verify_summary.txt`). `ftt` and `ftr` golden-v1 dumps identical.

## Screens (sequential, alternating, golden-v1, 20 passes, 4 rounds)
    pc [7.461, 7.472, 7.448, 7.515] median 7.466
    ftt [7.327, 7.315, 7.336, 7.304] median 7.321
    ftt vs pc per round [1.0183, 1.0215, 1.0153, 1.0289]
    ftt [7.314, 7.31, 7.279, 7.333] median 7.312
    ftr [7.24, 7.282, 7.257, 7.244] median 7.25
    ftr vs ftt per round [1.0102, 1.0038, 1.003, 1.0123]

`ftt` +1.5–2.9% vs `pc`; `ftr` a further +0.3–1.2%. Memory +56 MB (table) over `pc` (steady ≈1.87 GB, to be measured in validation).

## Follow-up: reuse of context-fill vocabulary probes in the feature memo (`EXP_FTT_MEMO`, build `ftm`) — rejected
Outputs identical (golden-v1, novel, OSM, 2M fuzz seed 2; `raw/verify_ftm.txt`); screen vs `ftr` (`raw/screen3_analyze.stdout`):
    ftr [7.209, 7.226, 7.258, 7.237] median 7.232
    ftm [7.315, 7.305, 7.293, 7.289] median 7.299
    ftm vs ftr per round [0.9855, 0.9892, 0.9952, 0.9929]
0.5–1.5% slower (the per-token byte comparison and status bookkeeping cost more than the warm probe saved). `ftr` remains the stack head. Source before this change: `raw/address_parser_c_ftr.c`.
