# Overnight summary — 2026-10-03 23:04:16 → 2026-10-04T03:17:13-0700 (ACTIVITY_LOG)

## Result
New deliverable **`cpi`** (`research/final/119-cpi`), measured against the original in the same session on the cleaned machine (experiment 121):

| set | original µs | cpi µs | speedup |
|---|---:|---:|---:|
| golden-v1 (12,863) | 31.37 | 5.17 | **6.08×** |
| novel (12,863) | 32.04 | 5.40 | **5.93×** |
| OSM (11,405) | 19.32 | 3.33 | **5.78×** |
| unseen golden-v10 (8,567; 120) | 17.50 | 3.08 | **5.68×** |
| unseen golden-v11 (7,882; 134, confirmation) | 17.85 | 3.17 | **5.64×** |

- **Output:** byte-identical to the original on every set, both frozen references, and 2 × 2M fuzzed inputs.
- **Threads:** one.
- **Memory:** peak 2.543 vs 2.614 GB; steady 1,927 vs 2,492 MB.
- **Load time:** about 12 s, against the original's 3 s (see the warning below).

## What changed overnight (all exact; each passed screen, stage timers and a never-used OSM set)
- **114 (`rt8`):** prefetch every cache line of transition-weight rows. +2.8% golden-v1, neutral on OSM.
- **119 (`cpi`):** all 4.01M place names in the parser's dictionary are enumerated at load, and their feature ids are stored. When an address matches one, the parser uses the ids instead of building and looking up feature strings. A length check proves the match equals the stored name; this was re-verified 12.8M times. +2.7% golden-v1, +5.6% OSM, +4.7% on unseen data. It is the first gain that is larger on OSM-style addresses.

## Tried and not kept (all recorded)
- **No measurable effect:** 105 (postcode skip flags), 111 (digit table), 128 (row-wise score clearing).
- **Faster on golden-v1, slower on OSM:** 113 and 126 (prefetches).
- **No net mechanism:** 122 (bigram integer keys).
- **Below the 1.5% threshold:** 130 (dictionary-phrase ids) and 133 (phrase join).
- **Stopped by its kill rule:** the bigram integer path (131).
- **Diagnostics:** 106–110, 112, 117, 118, 123–125, 127, 129 and 132, including a new cold-minus-warm *line-level* profiler (xctrace + atos).

## Things I did on the machine (you said to decide myself)
- Killed two orphaned VS Code crash reporters, PIDs 52927 and 54279. They had been using about 170% CPU each for more than 30 days and belonged to an old translocated VS Code copy that wasn't running; your VS Code in `/Applications` was not touched.
- Stopped a leftover AddressSanitizer test binary `t` (PID 38042). An earlier Claude session in this project started it on 2026-09-28.
- Restarted `icdd` (Image Capture), which had been at about 94% CPU for 3 days.
- **Effect:** every binary got 7–14% faster. Earlier results stay valid as same-session comparisons, but before-and-after absolute numbers differ, and the report says so.
- Started `caffeinate` for 48 h (PID 21869); stop it with `kill 21869` when you no longer need it.

## Honest status vs the 7.8× target
- golden-v1 would need 31.37 / 7.8 = **4.02 µs**, against cpi's 5.17 µs (−22% more).
- OSM would need **2.48 µs**, against 3.33 µs (−26%).
- Every lever still identified measures at about 1–3%.
- **7.8× was not reached, and it is not in sight with the methods found so far.** The most promising remaining direction is more of 119's kind (model-derived feature ids). Its largest remaining family, word bigrams, failed its probe (131).

## Before presenting
- **Load time:** cpi takes about 12 s to load against the original's 3 s. Parsing 12k addresses takes well under a second. If someone (or an AI assistant) times a whole run on a small file, the original will look faster. Report setup and parse time separately; the planned one-command benchmark must print both.
- **Still to fix for packaging:** the 091 aarch64 guard, and the hard-coded paths in `build_cc.sh`.
- **Unseen sets:** golden-v12 is built and unused, reserved for the next candidate. v8–v11 are spent.

## Addendum (afternoon 2026-10-04, user-directed)
All of the following were exact; none was faster, so cpi stays the deliverable:
- **Weight compression (135):** the weights are incompressible.
- **Hot tier (136/137):** about 88% of transition records hot, but no speed gain.
- **Next-address lookahead (138):** −3…−7%.
- **Full cross-address interleaving with coroutines (139/140):** the hardware can overlap waits (Phase 0), but the parser got 3–10% slower in every configuration.
