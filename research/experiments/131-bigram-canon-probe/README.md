# Experiment 131 — how often both bigram parts are canonicalized without a fresh vocabulary lookup (diagnostic)

Date/time/timezone: created 2026-10-04T03:03:28-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete — **bigram integer path stopped (kill rule)**

Build `bg` = cpi + `EXP_INT_KEYS_PAIRS` + `EXP_BG_PROBE` (`sources/address_parser.c`), no PGO. The branch of `ap_canon_part` taken for each bigram part was counted (`raw/probe_*.stderr`):
- **Development:** 98,524 emissions; both parts without a fresh lookup **37.1%**; part branches: vocabulary memo 89,789, word memo cached 30,485, word memo computed 20,693, placeholder 1,813, **fresh `fd_canon` 54,268**.
- **OSM:** 96,681 emissions; **38.6%**; fresh 51,288.
Most fresh lookups are the non-last part, whose trailing-separator form is not memoized. Under the pre-set kill rule (< 60%), a cached-only integer path for bigrams is not built. Caching the non-last canon per memo entry would roughly halve the fresh lookups, which is still not enough to clear the rule.
