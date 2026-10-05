# Can this parser reach 7.8× under the project rules? (update, morning of 2026-10-03)

Supersedes the estimates in `REACHABILITY_2026-10-02.md`, which were built on `l3`-era numbers. Every number here is MEASURED (with its experiment) unless labelled.

## Where we are
Deliverable `pcp` (`final/077-pcp`, 082/080/086): 5.15× golden-v1, 5.07× novel, 5.06× OSM, 5.04× and 5.06× on two unseen OpenStreetMap sets. Target 7.8× = 4.28 µs golden-v1 (pcp 6.48 → −34%), 2.70 µs OSM (4.17 → −35%), 2.50 µs unseen v6 (3.85 → −35%).

## The decisive measurement (088 warm bound)
Parsing each address a second time, with its data already in cache, gives the speed of the current code with zero memory stalls — the ceiling for every prefetch, layout or table-size change:

| set | pcp | warm bound | warm bound × | first-touch share |
|---|---:|---:|---:|---:|
| golden-v1 | 6.53 µs | 3.01 µs | 11.1× | 54% |
| OSM | 4.22 µs | 2.60 µs | 8.1× | 38% |
| unseen golden-v6 | 3.93 µs | 2.54 µs | **7.7×** | 35% |

On OpenStreetMap-style addresses the code would miss 7.8× **even if every memory stall disappeared**. Reaching the target there requires cutting the warm computation itself (tokenizing, normalization, phrase searches, building ≈55 feature keys, hashing and comparing them, ≈1,000 weight additions, Viterbi), not only memory traffic.

## Stage split of pcp (085 stage timers, development / OSM, cold-in-run vs warm µs)
| stage | dev cold | dev warm | OSM cold | OSM warm |
|---|---:|---:|---:|---:|
| context fill | 2.13 | 0.52 | 1.26 | 0.47 |
| feature generation | 1.56 | 0.81 | 1.02 | 0.66 |
| lookups | 0.85 | 0.27 | 0.52 | 0.22 |
| accumulation | 1.11 | 0.52 | 0.72 | 0.48 |
| Viterbi + rest | 0.66 | 0.62 | 0.58 | 0.55 |
(timer overhead included; tokenize/normalize/response ≈0.5 µs more.)

## What the night's experiments showed (070–087)
About 15 exact changes tried; 2 accepted. Accepted: 074 (prefetch pre-pass for first-token table entries and trie nodes, +2.4% golden-v1) and 077 (postcode-context row prefetch, +1.35%); together with 067/072, pcp is +4.8% over lf on golden-v1 but +2.3–3.0% on OSM-style sets (083). Rejected: compiler re-tune (070), certified early exit (071: decision margins 20–60× smaller than sound bounds), vocabulary prefetch/eager memo (075, 078), push-time prefetch (081), early postal prefetch (084), multi-line row prefetch (085: +4.7% golden-v1 but −1.6% on unseen v6), phrase-type prefetch (079: golden-v1 only), dense rows (087: rows 74/400 full). The only wins were dependent loads serialized a full stage behind their use; all first-touch buckets the timers expose have now been tried.

## Conclusion (for the user's decision)
- 7.8× on golden-v1 is not excluded by the warm bound (11.1×) but requires removing ≈90% of the remaining first-touch cost; every latency-hiding lever found so far has been tried.
- 7.8× on OpenStreetMap-style data (OSM, unseen sets) is **not reachable by memory-side work at all** (warm bound 7.7–8.1×); it needs the per-address computation reduced by about a third as well — the integer-pipeline / feature-engine rewrite estimated in the 2026-10-02 analysis (weeks, with exactness risk across tokenizer, normalization and feature generation).
- At the incremental rate measured tonight (≈1–2% per accepted change, about one acceptance in seven attempts, smaller on unseen data), the remaining ≈52% is not reachable by continued micro-optimization in a practical number of sessions. This is the "not worth it by incremental means" case of the user's stopping rule, unless a rewrite of the front half is commissioned.
