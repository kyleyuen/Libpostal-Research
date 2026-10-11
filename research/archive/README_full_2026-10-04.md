# Making libpostal's address parser faster — results and how we got here

> Historical snapshot, preserved for the 2026-10-04 results. Later measurements and coverage changed the project status; any ceiling or unseen-set statements below are retired and must not be treated as current findings. Use the current [root README](../../README.md) and linked outcome report for the verified status.

**Question:** can libpostal's address parser run **3.5× faster on one thread**, with **byte-identical output** and **no more memory** than the original?

**Where we are (measured 2026-10-04 01:38–01:50 PDT on a cleaned machine in [experiment 121](research/experiments/121-validate-cpi/README.md); unseen sets in [115](research/experiments/115-unseen9-remeasure/README.md), [120](research/experiments/120-unseen10-remeasure/README.md); AC, sequential timing, 5 rounds):**

| Input set | Original | `rt8` (previous) | Final build (`cpi`) | Speedup |
|---|---:|---:|---:|---:|
| golden-v1, all 12,863 real addresses | 31.37 µs/address | 5.33 µs | 5.17 µs | **6.08×** |
| novel set, 12,863 new strings derived from the same public records | 32.04 µs | 5.55 µs | 5.40 µs | **5.93×** |
| 11,405 OpenStreetMap addresses, 49 countries (different source) | 19.32 µs | 3.52 µs | 3.33 µs | **5.78×** |
| **unseen golden-v10: 8,567 OSM addresses from 53 cities in no other set, never used before** ([120](research/experiments/120-unseen10-remeasure/README.md)) | 17.50 µs | 3.21 µs | 3.08 µs | **5.68×** |
| unseen golden-v11: 7,882 OSM addresses from 51 more new cities ([134](research/experiments/134-unseen11-confirm-cpi/README.md), confirmation only) | 17.85 µs | — | 3.17 µs | **5.64×** |
| unseen golden-v9 (`rt8`; [115](research/experiments/115-unseen9-remeasure/README.md)) | 18.37 µs | 3.47 µs | — | 5.28× (`rt8`) |

Output is byte-identical to the original on every set, both frozen reference files and 4 million fuzzed inputs. Parsing uses one thread. Peak memory is 2.543 GB vs 2.614 GB; steady footprint is 1,927 MB vs 2,492 MB. Load time is about 12 s (original about 3 s). Package: `research/final/119-cpi/`.
`cpi` adds to `rt8` the first piece of the "integer front half" ([119](research/experiments/119-component-phrase-ids/README.md)):
- **Load.** All 4.01 million place names in the parser's component-phrase dictionary are enumerated. The model's feature ids for the features each name produces are looked up once and stored (48 MB).
- **Parse.** When an address matches one of those names, the stored ids are used directly instead of building feature strings and looking them up.
- **Exactness.** A length check proves the matched text equals the stored name. Every use was re-verified against the original string path, 12.8 million times.
- **Effect.** +2.7% golden-v1, +5.6% OSM, +4.7% on unseen data. It helps most on OpenStreetMap-style addresses, where the memory-side changes helped least.
`rt8` adds to `psf` one change ([114](research/experiments/114-trans-rows-prefetch/README.md)): a hint that fetches every cache line of each transition-weight row early, not only the first. It was found with a new cold-minus-warm line profile ([112](research/experiments/112-cold-warm-profile/README.md)). It gains 2.8% on golden-v1-style addresses and is neutral on OpenStreetMap-style ones (+0.6% OSM, +0.7% unseen v9). The same idea applied to all weight rows was rejected because it slowed OSM data ([085](research/experiments/085-row-lines-prefetch/README.md), [113](research/experiments/113-row-lines-128/README.md)).

**About measurement sessions.** On 2026-10-03 at about 23:30 we found four long-running background CPU users. Two orphaned VS Code crash reporters had been using about 170% CPU each for more than 30 days, so they were present in all earlier measurements. They were removed ([106](research/experiments/106-aa-noise-check/README.md)). Every binary got faster: the original by 7–10% and the optimized builds by 12–14%. Results from before and after the cleanup each compare binaries within one session; expect different absolute numbers on another Mac.
`psf` adds to `vmx` two exact changes from 2026-10-03 evening, each confirmed by stage timers and then on the unseen golden-v8 set before acceptance:
- **[098](research/experiments/098-norm-pair/README.md):** each token's standard and admin-form normalizations come from one scan instead of two; the parser's two option sets differ only in digit replacement. The token-normalization step is 40% faster. Over `vmx`: +1.1% golden-v1, +1.5% unseen v8.
- **[101](research/experiments/101-prefix-suffix-flags/README.md):** at load, the address-dictionary prefix and suffix searches run once on every vocabulary word. Two unused bits of the existing first-token-table entry record "returns nothing". At parse time, about 60% of these searches are skipped. Context fill is 0.11 µs faster; there is no extra memory. Over 098: +1.7% golden-v1, +3.1% unseen v8.
A third, exact change (099, joining phrase strings with known lengths) had a real but sub-noise effect and was not kept.
`vmx` added to `pcp` one exact change ([091](research/experiments/091-viterbi-maxonly/README.md)): the Viterbi forward pass for the model's 20 labels keeps only the running maxima (a hardware max instruction) and recovers each step's best predecessor on the decoded path afterwards by recomputing the same scores in the same order — the original's first-strictly-greater rule (Viterbi time −12%; +1.4% / +3.3% over `pcp` on golden-v1 / unseen v7).
`pcp` adds to `lf` four exact changes from the night of 2026-10-03: a per-address phrase-string cache ([067](research/experiments/067-phrase-memo-len/README.md)), a digit-only token fast path ([072](research/experiments/072-micro-bundle/README.md); exact but speed-neutral by stage timers, [073](research/experiments/073-profile-head/README.md)), and two prefetch passes that issue independent memory reads early so their cache misses overlap — the first-token table entries and trie nodes of the city/region phrase search ([074](research/experiments/074-ftt-prefetch/README.md), +2.4%, context fill −0.14 µs) and the postcode-context rows ([077](research/experiments/077-pc-prefetch/README.md), +1.35%, postcode features −0.16 µs). The per-change percentages are golden-v1 screen results; the combined gain of `pcp` over `lf` is +4.8% golden-v1, +4.6% novel, +3.0% OSM and **+2.3% on unseen golden-v5**, so it is smaller on OpenStreetMap-style data; counters show the unseen set has as many prefetch targets per address, so the difference is not fewer targets ([083](research/experiments/083-workload-check/README.md); likely more of these lines already cached on OSM-style data, unverified). Both changes were found by re-profiling the current build (073, 076) and confirmed by stage timers and on unseen data; a third prefetch that helped golden-v1 only was rejected when unseen data reversed it (−0.6%, about the noise floor; a workload-dependent hint, not a fitted one; [079](research/experiments/079-types-prefetch/README.md)). Earlier latency-hiding attempts (004, 012, 027, 037, 046) issued hints right before, or within the same loop as, the use; 074 and 077 issue them a whole stage ahead of a serialized use, which is why they help where those did not.
`lf` (4.92× / 4.86× / 4.91× in experiment 064) added to `va` a losslessly gap-encoded
postcode-context graph that frees 323 MB ([053](research/experiments/053-postcode-graph-compress/README.md)), a model-derived first-token table for the
city/region phrase search and an early role-row prefetch ([056](research/experiments/056-first-token-table/README.md)), 1-byte name ids in feature keys
instead of name text ([059](research/experiments/059-name-byte-keys/README.md), [061](research/experiments/061-name-byte-all/README.md)), and an exact
all-letters fast path in token normalization ([062](research/experiments/062-letters-fastpath/README.md)). `va` itself added a Viterbi decoder specialized
for the model's 20 labels and removed a per-address matrix copy ([048](research/experiments/048-viterbi20-alias/README.md)).

**Unseen-data check ([069](research/experiments/069-unseen-remeasure/README.md), 2026-10-02 23:52–23:56):** on a newly built set of 10,293 OpenStreetMap addresses from 56 cities used by no earlier dataset (never used for tuning or selection), `lf` is byte-identical to the original and **4.96×** faster (5 paired rounds); the next candidate `pl` (+ experiment 067) is 5.02×. The speedups hold on data the code has never seen.

**Earlier deliverable `l3` and why it replaced `l4`:** `l4` (the previous headline, 4.33× golden-v1 / 4.36× novel / 4.31× OSM in 021) uses sparser lookup tables. A 2026-10-02
same-session check ([031](research/experiments/031-pgo-crosstrain/README.md)) found the original's steady footprint at 2,172 MB (it was 2,492 MB in the 021
session) and `l4` at 2,250 MB, i.e. 78 MB above the original. `l3` is the same code with slightly denser tables; it is as fast as `l4` within noise
(same session: 4.38× vs 4.37×, 4.33× vs 4.32×, 4.28× vs 4.34×) and stays below the original on peak and steady memory. Earlier 021 measurements of `l4`
on the floor (non-ASCII, 4.27×) and ASCII-only (4.30×) subsets are kept in the experiment records; they were not re-run for `l3`.
The OSM set was used during 020/021 selection, so it is not a strict held-out estimate.

**2026-10-04 evening: ceiling re-measured on the current deliverable `cpi` ([141](research/experiments/141-warm-bound-cpi/README.md), diagnostic; the headline speedups above are unchanged).**
- The warm bound (each address parsed a second time with its data already cached) is now **12.9× on golden-v1 development, 8.6× on OSM and 8.5× on unseen golden-v11**. It was 11.1 / 8.1 / 7.7× on `pcp` (088, below).
- The earlier statement that 7.8× on OpenStreetMap-style data is out of reach even with every memory stall removed therefore **no longer holds**. The target is now 9–10% inside the bound, but reaching it would still need almost all first-touch cost removed, or the per-address computation cut.
- About 0.8–0.9 µs per address (accumulation + Viterbi) is fixed by the byte-identity rule.
- A rewrite plan for the remaining front half is in [REWRITE_PLAN_2026-10-04](research/plans/REWRITE_PLAN_2026-10-04.md). It is not started, and its estimates are unverified.

**2026-10-03 morning — how far this can go, updated ([REACHABILITY_2026-10-03](research/analysis/REACHABILITY_2026-10-03.md)):** the warm bound of `pcp` itself ([088](research/experiments/088-warm-bound-pcp/README.md): each address parsed a second time with its data already cached) is 11.1× on golden-v1 but only 8.1× on OSM and **7.7× on unseen golden-v6**. On OpenStreetMap-style addresses the 7.8× target is therefore out of reach even if every memory stall disappeared; it would need the per-address computation itself cut by about a third (a rewrite of tokenizing, normalization and feature generation as an integer pipeline). Overnight, about 15 exact changes were tried and 2 accepted ([070](research/experiments/070-compiler-retune/README.md)–[088](research/experiments/088-warm-bound-pcp/README.md)); rejected ones include certified early exit (071: the best label path wins by margins 20–60× smaller than any sound bound on the skipped features), multi-line weight-row prefetch (085: +4.7% golden-v1 but −1.6% on unseen data, [086](research/experiments/086-unseen6-remeasure/README.md)), early postal-trie prefetch (084), push-time and vocabulary prefetches (075, 078, 081) and dense weight rows (087).

**2026-10-02 evening — how far this can go ([reachability analysis](research/analysis/REACHABILITY_2026-10-02.md)):** the realistic ceiling (8.7×) and the warm bound (9.2×, the same code with an address's data already cached) show more speed is possible in principle. Measurements 041–050 show that about half of the remaining time is first-touch memory cost spread over many model structures and the rest is compute spread over many stages; five latency-hiding attempts and several layout/bookkeeping changes did not help, and the optimistic sum of every identified lever (≈2.65 µs) falls short of the ≈3.15 µs needed for 7.8×, requiring a ground-up rewrite. These are estimates, not results.

**2026-10-02 measured ceiling ([035](research/experiments/035-ceiling-oracle/README.md); a diagnostic probe, not a result):** replaying each address's final feature ids so that context fill, feature generation and lookups cost nothing gives 2.63 µs/address (13.0×) on golden-v1, 13.1× on novel and 11.4× on OSM, against 4.2× for the same build without the replay. Today about 5.5 µs of the ~8 µs per address is that front half; the remaining ~2.6 µs (tokenizing, weight accumulation, Viterbi, response) is the floor of the current back half. Reaching most of the ceiling would require rewriting the front half as an integer pipeline. The ceiling relies on a cross-address cache forbidden by the rules and is reported only as a bound. That probe also skipped the dictionary lookups every legitimate design must still pay; a second probe that replays the feature keys but still performs the lookups gives the **realistic ceiling: 3.91 µs (8.7×) golden-v1, 8.6× novel, 7.6× OSM**. Two follow-up attempts at the phrase-search part of the front half (036–037) gave no speedup (037 was 3–5% slower and was rejected).

**2026-10-02 PGO generality check ([031](research/experiments/031-pgo-crosstrain/README.md)):** `l4`'s profile-guided optimization is trained on golden-v1 development addresses. To test whether that tunes the build to our data, the same sources were rebuilt with PGO trained on golden-v1, novel or OSM, and without PGO, and every build was timed on every dataset (5 paired rounds). No build is faster on its own training set (largest difference +1.1%, within noise); speedups on datasets a build never saw are 4.31–4.33× (golden-v1), 4.36–4.37× (novel) and 4.29–4.31× (OSM), equal to `l4`. Without PGO the speedup is 4.11–4.28×. The same session found the original's steady footprint at 2,172 MB (vs 2,492 MB in 021) against `l4`'s stable 2,250 MB, so the steady-memory claim is qualified above; peak memory remains 142 MB below the original.

**2026-10-01 audit:** [the consolidated source and experiment audit](research/analysis/FINAL_AUDIT_2026-10-01.md) confirms the core result, identifies a reproducible crash in optional feature-print mode, and records an unsuccessful further speed attempt in [025](research/experiments/025-audit-next/README.md). The repaired build is a separate maintenance candidate; 021 `l4` remained the measured speed deliverable (superseded by `l3` on 2026-10-02, see 032).

**Cross-platform follow-up paused:** [026](research/experiments/026-cross-platform/README.md) confirms byte-identical output and one-thread parsing for the 021 source on Linux ARM64 inside Docker Desktop. The exact Mac dictionary settings fail Linux's memory limit; a denser dictionary variant passes paired peak and late memory checks on golden-v1, novel and OSM by about 14–15 MB. A stopped, heavily contended golden-v1 diagnostic showed a 3.5225× paired warm median for that dense variant, but lacked novel and true cold timing; **no Linux speed result is accepted**. Setup takes about 10.1 s versus 2.46 s for the original on Linux golden-v1. Under x86-64 Linux emulation, original and candidate builds with SSE2 also produce byte-identical output on all three datasets; emulation is not speed evidence. Windows x64 builds cross-compile, but no Windows runtime is available. A profile of the actual Mac `l4` build led to a separate [027 root-transition experiment](research/experiments/027-root-transition/README.md), which showed no development-set speed gain and was rejected. These findings do not change the 4.33× macOS headline.

## The rules every change had to pass

1. **Byte-identical output**: every (label, component) pair for every input must equal the original library's — checked against the original's dump,
   the frozen reference files, a negative control, and 2 × 2,000,000 fuzzed inputs. Many changes also have a lookup- or function-level differential test.
2. **One thread**: no threads, no second process (checked with `ps -M`).
3. **No more memory** than the original (peak and steady footprint).
4. **Nothing learned from the test data**: tables come from the model files or the code; no caching of results across addresses.
5. **Fair timing**: original and candidates in the same session, on AC power, interleaved order, many passes, on the full sets and on an unseen set.

Full protocol: [research/METHOD.md](research/METHOD.md).

## How we got here (every experiment, including the ones that failed)

| # | What changed | Result |
|---|---|---|
| 000-002 | Baseline measured; per-stage timing showed the model's feature lookups dominate and wait on memory | baseline 35 µs/address |
| 003 | Look up all of a token's features before adding weights (more lookups in flight at once) | 1.14× — accepted |
| 004 | Interleaved trie walks with prefetching | no gain — rejected |
| 005 | Remove redundant work (byte-by-byte copies, double zeroing, duplicate lookups) | +4.4% — accepted |
| 006 | Replace the two feature tries (~21 dependent memory reads per lookup) with an exact hash dictionary built from the model at load | 1.78× — accepted |
| 007 | Controlled re-measurement of everything so far | confirmed |
| 009 | Build feature keys directly in one buffer; prefetch weight rows | 2.07× — accepted |
| 010 | Vocabulary trie → exact dictionary; faster key joining; lean decoder setup | 2.37× — accepted |
| 011 | Exact ASCII fast path for string normalization (proved from the transliteration rule tables) | 2.66× — accepted |
| 012 | Inline hash/compare, reordered Viterbi loops, phrase-search prefetch | all slower — rejected |
| 013 | Speed on random subsets and on the hardest inputs | 2.64× full set, 2.33× floor |
| 014 | Measurements only: memory latency, cache lines per address, vocabulary coverage | estimated memory floor ≈ 0.3-1.2 µs in the then-current build; not a proof for 021 |
| 015 | Look up word features by integer ids instead of strings | 2.75× / 2.74× (+2.1-2.6%) — accepted |
| 016 | Fast path for non-ASCII normalization, phrase prefilter, word memo, lean buffers, profile-guided + link-time compilation | 3.26× / 3.27×, floor 3.24× — accepted (normalization and PGO/LTO carry the gain; three changes gave nothing) |
| 017 | Profile-guided: cache a constant lookup; build each response in one allocation (rest of the load-time plan still open) | **3.42× / 3.42×, floor 3.35×** — accepted |
| 018 | Compiler only: whole-library profile-guided + link-time optimization, optimized tokenizer (upstream builds it unoptimized) | **3.39× / 3.39×, floor 3.30×** — accepted |
| 019 | Reuse per-address objects instead of allocating them; inline dictionary compare (under whole-library optimization); compile-time key-name lengths; Korean syllables in the normalization table; packed weight records (−430 MB peak memory) | **3.84× / 3.85×, floor 3.78×, independent OSM set 3.76×** — accepted |
| 020 | Other compilers (clang 23, GCC 16) and allocator (mimalloc); timing of 019's follow-ups: per-word feature ids precomputed at load (`q1`/`q2`) | **≈4.0× on every dataset** (`q2`); other compilers slower, mimalloc +1%, phrase prefilters −2.5% |
| 021 | Look up constant features (`bias`) once at load; link the faster mimalloc allocator; feature dictionaries at 25% load and vocabulary at 50% (much shorter probe chains on misses); skip clearing an unused score row | **4.33× / 4.36×, floor 4.27×, OSM 4.31×, ASCII 4.30×** — accepted, final |
| 022-024 | Portable build and memory diagnosis; allocator pressure-relief attempt | portable build is fast but fails memory; pressure relief had no effect |
| 025 | Source audit, packed-row API repair, and no-copy ASCII normalization attempt | repair passes output checks; ASCII speed attempt rejected; 021 headline unchanged |

The ledger with every number and link is [research/experiments/README.md](research/experiments/README.md); each experiment folder holds its
hypothesis, source, commands, raw runs and checks.

## What we learned

- **The big wins came from changing data structures and removing work**, not from tuning instructions: tries → hash dictionaries (006, 010) and a proven
  fast path that skips normalization for plain ASCII (011).
- **Micro-optimizations often made things slower** (004, 012). A profile's share of time overstates what removing that code saves (015 removed string handling
  from half the lookups and gained 2%).
- Experiment 014 estimated a modest memory-latency floor for the then-current build. Its profile does not establish the bottleneck in 021; a fresh profile is needed before choosing the next change.
- **The hardest inputs are hard because of text normalization, not the model**: non-ASCII addresses have the same number of tokens and features as ASCII
  ones (016 T1); the extra ~3 µs is in normalizing the text.

## Experiment 016 — what each change was worth (measured)

| Change | golden-v1 vs 015 | Note |
|---|---:|---|
| T4 skip transliteration steps that provably cannot match | +5.0% | exact (6 M differential strings) |
| T4 + T5 one-pass normalization table for 42,532 "context-free" characters | **+11.2%** | normalization on non-ASCII addresses 2.82 → 0.16 µs; floor 2.38× → 2.97× |
| T6 phrase-search prefilter | +1.3% | exact, but the skipped walks were already cheap |
| T7 compute neighbouring word details once | −0.7% | no gain |
| T8 avoid re-copying / fully re-clearing score matrices | −0.2% | bookkeeping cost what the skipped work saved |
| T11 profile-guided + link-time compilation (trained on the development split only) | **+10%** on top | no source change; byte-identical |
| **All together (`pgolto`)** | **+20%** | **3.26× vs the original** |

A 20-round paired run also confirmed 015's gain (+2.5%, faster in 19 of 20 rounds). Remaining time per address (full set): phrase/context search
3.2 µs, feature building 2.7 µs, lookups 2.2 µs, weight accumulation 1.2 µs, decoding 1.0 µs, response 0.8 µs, tokenizing 0.5 µs, normalization 0.1 µs.
Experiment 018 then compiled only the useful changes (T4+T5) with whole-library profile-guided and link-time optimization and an optimized tokenizer:
+3-5% more, no source change. Next: experiment 017 (spend load time on precomputed tables) against context search, feature building and lookups.

## Where things are

```text
README.md                   this report
research/
  METHOD.md                 measurement and acceptance rules
  ACTIVITY_LOG.md           chronological record of all work, including mistakes
  HANDOFF.md                current state, open items and practical notes for the next session
  experiments/              ledger (README.md) + one folder per experiment (000-028) with sources, raw data and checks
  analysis/                 FINAL_AUDIT_2026-10-01.md, earlier function audit, algorithm research
  final/                    packaged builds: 021-l4 (final candidate: sources, build script, hashes, BUILD.md) and the earlier 011 candidate
  tools/                    shared benchmark, fuzz and environment-gate tools
  archive/                  superseded summaries (earlier professor report, 011 candidate summary, organization manifest)
profiling/                  original baseline evidence and the frozen datasets (golden-v1, golden-v2-novel) — never modified
libpostal/                  library source checkout
AGENTS.md                   rules for keeping the experiment record auditable
```

Model files live outside the folder at `~/.cache/libpostal-profile/` (URLs and hashes in `profiling/manifest.json`). All accepted speed timings are from one Apple M4 Pro laptop (24 GB, shared, on AC power) with arm64 macOS executables. The Linux and Windows build/functional evidence in 026 is labeled separately.

## Honest limits

- Measured on one shared laptop; small effects (< ~1%) are within noise unless paired rounds are used.
- Upstream's [M1 `--disable-sse2` note](https://github.com/openvenues/libpostal/blob/master/README.md) concerns an x86 instruction path; the pinned [configure source](libpostal/configure.ac) already excludes SSE2 flags on ARM. It does not imply that Linux or Windows will be faster, or that this Mac's 4.33× relative gain transfers to another CPU.
- The datasets are public, country-skewed address sets; the novel strings are derived from the same public records, and OSM was used during final tuning. They do not prove a speedup on arbitrary future traffic.
- The three unseen sets (golden-v4, v5, v6) come from one generator (OpenStreetMap, city centres, four templates). They show the gains hold for new OSM cities, not for a new data source. Acceptance rule applied since 2026-10-03: faster on the screening sets and not slower on unseen OSM data. Gains are larger on the Senzing-derived sets (golden-v1, novel) than on OSM-style sets ([083](research/experiments/083-workload-check/README.md)).
- The original library crashes on some invalid UTF-8; that behaviour is untouched and such inputs are excluded from the fuzzer.
- Load time grows from 3.4 s to 8.7 s because dictionaries and tables are built from the model at start-up.
- **The accepted Mac memory result depends on mimalloc** (experiment 022): the same code with the macOS system allocator, no profile-guided or
  link-time optimization and no ARM-specific decoder (`pt`) is still 3.74-3.86× faster and byte-identical **on that Mac**, but uses 3.18 GB peak / 3.03 GB steady —
  more than the original. Experiment 026 found that the exact 021 source also exceeds the original's peak RSS on Linux ARM64 with both glibc and active mimalloc 3.5.3. A denser dictionary variant clears Linux memory in the tested VM. Windows allocator behavior and non-Mac speed remain unverified.
- Memory headroom is now 142 MB at peak (2.471 vs 2.613 GB): sparser tables trade spare memory for speed, inside the no-increase rule.
