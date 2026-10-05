# Experiment 100 — coverage probe for a vocabulary-slot prefix/suffix table (diagnostic, idea A)

Date/time/timezone: created 2026-10-03T22:17:02-0700; probe runs finished 22:17:39 (`raw/probe_v1.stderr` mtime), America/Los_Angeles. Status: complete (diagnostic; no timing)

Build `psp` = npr (098) sources + `EXP_PS_PROBE` counters (`sources/address_parser.c`), no PGO/LTO (`raw/commands.txt`). For every prefix/suffix search in context fill, the probe records:
- whether the raw token's bytes equal its admin-normalized bytes;
- whether those bytes are a vocabulary key;
- whether each result is NULL_PHRASE.
At load it also runs both searches (lang NULL, as at the call site) over every vocabulary key. Outputs: `raw/probe_*.stderr`.

| set | calls | raw = admin | eligible (also in vocab) | both null | eligible, both null | eligible, ≥1 null |
|---|---:|---:|---:|---:|---:|---:|
| development | 60,596 | 60,324 | 39,162 (64.6%) | 55,993 | 34,886 (57.6%) | 39,143 |
| OSM | 61,157 | 60,757 | 39,968 (65.4%) | 57,264 | 36,332 (59.4%) | 39,962 |
| golden-v1 all | 75,659 | 75,318 | 48,899 (64.6%) | 69,930 | 43,572 (57.6%) | 48,870 |

Vocabulary: 3,694,560 words. 39,060 have a non-null prefix result, 767,813 a non-null suffix result, and 802,084 either. The load-time pass takes 0.50 s.
Interpretation: about 65% of the searches could be answered by a model-derived flag; the rest are mostly tokens absent from the vocabulary (numbers keep digits in the raw form). The FTT entries (056) have 9 unused bits (54–62), so two "result is null" flags fit with **no extra memory**, in an entry the slot loop already fetches and 074 prefetches. This avoids 063's failure mode (a separate cold table). → 101.
