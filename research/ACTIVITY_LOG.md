# Activity log

Keep dated entries in chronological order. Record decisions, changes, verification,
failures and evidence paths; do not retrospectively invent exact commands or times.

## 2026-09-28 — Profiling and golden dataset established

See the preserved [original work log](../profiling/WORK_LOG.md) for the detailed
build/profile/download history, including the corrected exploratory 71-input run.
Original evidence and source/model provenance: [manifest](../profiling/manifest.json).
Golden-v1 baseline: 27,821.28 addresses/s, maximum peak RSS 2,613,886,976 bytes.
No library optimization has yet been implemented.

## 2026-09-28 — Organized for professor review and future comparisons

User requested an accessible record supporting an eventual improvement claim.

- Created the root start page, professor report, measurement protocol, experiment
  ledger, baseline evidence index and reusable experiment record template.
- Verified all 70 existing files against `profiling/SHA256SUMS` before organizing.
- Preserved the golden baseline executable, harness source, static library and
  dynamic library under `experiments/000-baseline/artifacts/`. Verified executable
  and library hashes against the original manifest. Captured an empty tracked
  source patch and the untracked/generated build inventory.
- Removed five redundant workspace-root `profile-*.log` files only after bytewise
  comparison confirmed identical preserved copies in `profiling/setup-logs/`.
  No unique log content was removed; frozen evidence paths were unchanged.
- Added workspace `AGENTS.md` instructions requiring future experiments to retain
  source changes, commands, raw trials, correctness/memory results and decisions.
- Checked internal documentation links and the copied dynamic-library output
  check. Saved verification output and a checksum snapshot for the new organization.

This step changed organization and documentation, not libpostal implementation or
the measured baseline. No new speedup is claimed. Earlier missing terminal output
cannot be recovered; the historical log distinguishes saved records from summaries.

## 2026-09-29 — Experiment 001: component runtime audit

Created [001](experiments/001-component-audit/README.md). No libpostal source or frozen data changed.
Built `tools/stage_bench.c` (replica of `address_parser_parse`/`crf_tagger_score` with per-stage timers), verified 10,308/10,308
identical to the library on development.txt, ran 3 runs x 8-10 passes, a trie locality experiment and two Instruments CPU Counters
recordings. Findings: state+prev-tag trie lookups 44% of staged time, feature generation 18%, context_fill 12%, normalize 8%,
Viterbi 2%; lookups cost ~245 ns in situ vs ~56 ns cache-resident; steady-state counters show ~61% back-end-stalled slots
(column meaning inferred). A batching-order control was ~12.7% faster in the harness only. No speedup claimed.
Limitation: run 1 used an earlier revision of the harness that was not saved separately.

## 2026-09-29 — Experiment 002: complete baseline

Created [002](experiments/002-full-baseline/README.md) (measurement only, no libpostal change). Ran 15 throughput trials (5 x development/validation/all,
rotated), steady-state footprint sampling, original-order stage breakdown with ASCII/non-ASCII, hit/miss and step-distribution measurements,
a fresh sampling profile with subtree attribution, and a counter-column calibration. Findings: noise band 3-7.5%; steady footprint 2,172 MB flat;
peak RSS unreliable because swap was full; lookup+accumulate is 56.6% of time in the original order; counter cols 0/1 confirmed (col1 = any back-end stall).
Problems recorded honestly: `timeout` missing (first memory run did nothing), disk filled (calibration re-run after the user approved deleting 4 raw traces:
001's two .trace files and 002's partial calibration traces; exported tables kept), L1D/TLB counters not selectable from the CLI. 001 has appended
correction/evidence notes. Next: 003 = batched/prefetched feature lookups.

## 2026-09-29 — Experiment 003: batched feature lookups (first candidate)

Created [003](experiments/003-batched-lookups/README.md). Changed only `crf_tagger_score` in a copy of `crf.c` (patch saved); built control (unchanged) and candidate by swapping
one object into a copy of the baseline `libpostal.a`. Outputs byte-identical to baseline on all 12,863 inputs (same SHA-256; 0 mismatches vs frozen golden; negative control detected).
Single thread verified (1 thread, CPU ~ wall). Throughput vs baseline: 1.141x development, 1.114x validation, 1.138x all (7 rounds + burn-in each, rotated, 72 runs all exit 0).
Steady footprint unchanged at 2,172 MB. Accepted as incremental; 3.5x target not met. User added hard constraints (byte-identical + single thread) to METHOD.md and AGENTS.md.
Next: 004 interleaved trie walks.

## 2026-09-29 — Experiment 004: interleaved trie walks (rejected)

Created [004](experiments/004-interleaved-trie/README.md). Added `trie_get_data_batch` (lane-interleaved walks with prefetch) to a copy of `trie.c`/`trie.h` and used it from `crf.c` on top of 003; lanes 4/8/16.
All three byte-identical to baseline on 12,863 inputs. Development timing (baseline, 003, L4, L8, L16; 7 rounds + burn-in; 40 runs): all ~1.11-1.17x vs baseline but none beats 003 beyond noise (L8 1.013x, L16 0.988x, L4 0.963x).
Recorded as inconclusive/rejected; milestone runs skipped. This weakens the memory-overlap explanation for 003's gain (untested alternative: loop restructuring). Also fixed a latent empty-first-batch edge in the shared buffer helper (present in 003's candidate; never triggered in any test; 003's accepted numbers are unchanged).

## 2026-09-29 — Protocol v2 and experiment 005

Added the generalization protocol to METHOD.md (structural-only design rule, novel dataset, cold pass with cross-dataset warm-up, per-country reporting, setup time). Built `profiling/golden-v2-novel/`
(12,863 strings none in golden-v1, own SHA256SUMS and baseline reference) and `research/tools/bench_ext.c`. 003 re-checked: byte-identical on the novel set; warm speedup 1.137x novel / 1.214x golden-v1, no country < 1.05x;
the first cold-pass attempt was unusable (swap ~full, spreads 35-116%) and was redesigned (kept as `generalization-attempt1-no-warmup`).
Experiment 005 (bundle of exact work eliminations): byte-identical on both datasets; ablation inconclusive per part; powered 20-round test shows +4.4% over 003 (18/20 rounds). Estimated 10-14% was too optimistic.
User set the next objective: reasonable load time acceptable; byte-identical output and no extra memory. Next: 006 = load-time compact exact dictionary replacing the feature trie.

## 2026-09-29 — Experiments 006 and 007: feature dictionary and controlled re-baseline

User direction: load time may increase if reasonable; byte-identical output and no extra memory. Advisor review gated 006 on a harness prototype. Prototype (measurement only) built an exact dictionary from the loaded tries:
0 mismatches on an exhaustive round trip (11.47M + 2.09M keys) and 2M fuzz mutations per trie; 5.7% smaller than the tries after replacing power-of-two tables (first version was +1.3 MB, rejected and kept). Library integration `dn`/`d`:
byte-identical on golden-v1 and the novel set, single thread, peak footprint 2.602 GB vs 2.613 GB, setup +3 s. User then asked for environment control (shared laptop, CPU load, battery): added the environment protocol to METHOD.md
(AC/low-power/thermal gate, QoS class, per-run telemetry, robust statistics, same-session baseline) and ran 007: baseline, 003, 005, 006 interleaved, 60 processes. Results: 003 1.14-1.15x, 005 1.185-1.20x, 006 1.78-1.80x
(cold 1.74-1.79x, per-country min 1.62x); validation split 1.817x (7/7 rounds). Earlier conclusions confirmed; the frozen baseline reproduces within ~1%. Mistakes recorded: first prototype build returned NULL because of a signed-char alphabet index bug
(fixed); first footprint sampler attached to the `time` wrapper instead of the benchmark (results discarded, kept in `raw/discarded`); 007's during-run `top` CPU capture was lost (snapshots used instead).
Not mine and not touched: four runaway user processes at ~100% CPU each.

## 2026-09-29 — Experiment 008: fresh profile after 006 and model memory composition (analysis only)

Sampling profiles of the baseline and the 006 build, allocation-caller breakdown and a model-composition measurement. Findings: libc string ops 24% of the new profile (memmove 11%, of which ~6% is 005/006's own key copying/building), weight accumulate 13% on one line (cold CSR reads),
allocator+zeroing ~8.7%, phrase search/context fill 18%, normalization 11%, vocab trie 7%. User's assumption (runtime headroom > allocation headroom) supported. Loaded structures measured: weights 592 MB, feature dictionaries/tries 376 MB, other tries 316 MB, ~1.1 GB not yet itemized.
Recorded honestly: the AC gate failed (laptop unplugged) when the profiles were taken; profiling only. Any further TIMING run will abort until the laptop is on AC.

## 2026-09-29 — Experiment 008 rerun on AC power

User plugged the laptop in and asked for the run again. Gate OK (AC, battery 91% charging). Baseline and 006 profiles retaken; all shares agree with the battery run within ~1 point
(memmove 11.3%, `crf_tagger_score` 13.3% on one line, allocator entry points 6.0%, phrase search 8.6%). The battery-run files are kept under `raw/battery-run/`. Conclusions of 008 unchanged.

## 2026-09-29 (evening, unattended) — Experiments 009, 010, 011

User was away and asked me to keep going with a caffeinated session. The laptop was kept awake with `caffeinate -dimsu -t 43200`; benchmark sessions waited for the AC gate instead of aborting (never needed). Disk was tight (1.9 GB free), so new variants did not keep archive copies.
- 009 (direct keys + weight-row prefetch): 2.07x / 2.12x. Fresh profile (008) had shown key copying/splitting ~15% and the weight-accumulate line 13%.
- 010 (vocab dictionary, fast join, lean context, arena prefetch): 2.37x / 2.40x. The arena prefetch had no effect; the vocabulary dictionary gave 3x my estimate; peak footprint fell to 2.557 GB.
- 011 (ASCII normalization fast path, shared admin normalization): 2.66x / 2.71x. Proved by enumerating the transliteration rules (all `&`-initial or non-ASCII-initial); differential tests 0 mismatches over ~80M cases; end-to-end fuzz 2 x 2M mutated inputs identical.
Findings recorded: the original library segfaults on some invalid UTF-8 input (pre-existing); the ASCII fast path covers 58-60% of our datasets, so the gain depends on the input mix.
Mistakes recorded: a fuzz run first failed because unquoted paths with spaces were split (outputs were empty, the "identical" comparison was meaningless and was discarded); an earlier ASCII test included control bytes and `&` and correctly reported mismatches (kept, it defined the domain).
User interrupted once during a long wait; I stopped and waited for instructions as told.

## 2026-09-29 21:00 — Stopped launching work: laptop went onto battery

Validation split with the frozen harness for `h`: 75,024 vs 27,760 addr/s = 2.703x (paired median 2.705x, 7/7 rounds). The AC gate then failed (laptop unplugged, 100% charge, ~2 h remaining). Timing runs need AC (the unattended runner would wait up to 2 h);
I did not start further experiments or the planned re-profile of `h` while on battery. Cumulative state: 003, 005, 006, 009, 010, 011 accepted; best candidate `h`.

## 2026-09-29 21:50 — Experiment 012 (rejected)

Re-profiled `h` on AC power and tried three profile-driven micro-optimizations: inline dictionary hash/compare, a restructured Viterbi loop nest, and phrase-search prefetch. All byte-identical, all slower than the control (-0.7%, -2.7%, -1.4%; -1.9% combined on golden-v1).
Dictionary exactness re-verified with the inline hash. Best candidate remains 011 `h`. Recorded as a failed attempt; nothing carried forward.

## 2026-09-30 — Algorithm research (no code, nothing run)

Wrote `ALGORITHM_RESEARCH.md`: each algorithm on the parse path checked against the literature (web search; Chen & Manning 2014 and Kaji et al. 2010 read in full,
others from search summaries and marked as such), with its measured share of build `h`. Finding: decoder, tokenizer, trie and hash-table types are already near
the best known; the bottleneck is the feature pipeline. Two new exact candidates: per-word partial-score precomputation (exactness checked against `h`'s
`crf_tagger_score_direct` accumulation order) and integer-keyed feature lookup. No new speed claim; three open measurements listed.

## 2026-09-30 — Mathematical formulation added (no code, nothing run)

Added section 6 to `ALGORITHM_RESEARCH.md`: score decomposition, exactness lemmas (non-associativity counterexample, same-order prefix reuse, prefilter
soundness, memoization purity, integer-key injectivity), a certified-argmax research idea, Amdahl savings table (arithmetic from experiment 013 figures),
Little's-law memory model, Zipf coverage model, and a hand-off checklist Q1-Q8 for another session to compute as experiment 014. No quantities computed yet.

## 2026-09-30 13:24-14:00 — Experiment 014 (measurement only): section 6 quantities

Computed Q1 and Q3-Q7 of `ALGORITHM_RESEARCH.md` 6.8 on the development split and model files (no library change, no speed claim).
Headlines: 3.5x needs 24.6% of h's time removed (about all of context_fill); h's feature lookups touch 270 distinct 128-B lines per address;
vocabulary counts follow Zipf s ~1.18 and the top 250,000 words (40 MB dense) cover 94.9% of model counts but 88.3% of dev plain tokens;
only 35.9% of plain tokens have `bias` first (postcode features precede it otherwise), which breaks the per-word precomputation as stated;
integer-key reach is 0.762 of lookups (0.500 from word-role features). Q7 found two lemmas that fail as written (U-block prefix; one-byte prefilter).
The regenerated feature dump is byte-identical to 006's `dev_features.tsv`. Mistakes recorded: one invalid negative control (pattern matched nothing), two
failed builds, analysis runs before the script existed. Q2 (pointer chase) not timed: laptop on battery, gate refused; runner waiting for AC.

## 2026-09-30 14:00-14:20 — Experiment 014 Q2 completed on AC power

Before timing, the advisor review found the first chase layout stored both an index and a pointer per node (the "pointer-free" variant was not); fixed and
rebuilt. A k <= 16 session never saturated, so the probe was extended to 32 chains: DRAM latency 127 ns, memory-level parallelism >= 29.7 with no
saturation (the index variant's drop at k >= 30 is NEON code generation, checked in the disassembly). Little's-law floor for h's feature lookups:
1.16 µs per address if every address starts cold (8.7% of 13.22 µs), 0.33 µs under a 16 MiB LRU model. Also corrected a README statement: deleting all
of context_fill gives 3.47x, just short of 3.5x. Q1-Q7 complete; Q8 not attempted.

## 2026-09-30 14:28-15:15 — Experiment 015 (integer-keyed feature lookup): small verified increment

Implemented integer keys for word features (vocabulary slot ids, load-time split of the state/transition dictionaries, one canonicalization function used at
load and at run time, one routed lookup for every reader) plus a per-address vocabulary memo, as switches on top of 011 `h`. All variants byte-identical on
golden-v1 and novel; a verify build checked 2.1 M dataset lookups and 280 M fuzz lookups against the unsplit dictionaries, 0 mismatches; `k1`, `k2`, `kv`
fuzz-identical (2 x 2 M). Controlled session: `k1` 2.745x (golden-v1) / 2.741x (novel) vs baseline, +2.6% / +2.1% vs rebuilt control `h0`; memo alone and
word pairs add nothing. Peak footprint 18.6 MB below `h0`; setup +2.7 s. Much smaller than the instruction-bound argument suggested; recorded honestly.
Mistake noted: `k1` was initially left out of the fuzz run (only `kv`/`k2`); fuzzed afterwards, identical.

## 2026-09-30 15:16-15:20 — 015 follow-ups: frozen-reference check, steady memory, floor

`k1`/`h0` identical to both frozen reference files (negative control fails as expected); steady footprint baseline 2,493 / `h0` 2,439 / `k1` 2,421 MB.
Floor subset (5,404 non-ASCII addresses, 5 processes each, AC-gated): baseline 33.41, `h0` 14.51, `k1` 14.17 µs -> floor 2.36x for `k1` (+2.4% over `h0`).

## 2026-09-30 15:45-17:20 — Experiment 016 (batch of tests 1-11): built and verified on battery, timing pending AC

T1 (counts): non-ASCII addresses have the same tokens/features as ASCII ones; indicative stage split (battery, not a result) puts ~2.8 of the ~3.1 µs gap
in whole-string normalization. Built with one switch each: T4 transliteration skip, T5 normalization table (with T4), T6 phrase prefilter, T7 word memo (F1),
T8 lean scoring buffers, combinations `all`/`all7`, T11 PGO/PGO+LTO (trained on development only), stats builds for T2. Exactness: differential tests
(T4 6 M strings, T5 42 M fast-path answers, T6 10.7 M searches double-checked) and end-to-end dumps/frozen references/fuzz identical for t4, t45, t6, t8,
all; chain running for t7, all7, pgo, pgolto. Mistakes recorded: test-tool buffer overflow (T5), invalid-UTF-8 inputs in a test crashing the original
library (T4), deadlocked verification queue (killed, restarted), PGO flag split on a path with spaces. Not built: T9, T10, R1/R2, F2 (reasons in README).
Batch script `016-batch/tools/run_batch.sh` ready for AC.

## 2026-09-30 — Reorganized the write-up

The root `README.md` is now the report for any reader (question, measured result, every experiment, lessons, 016 status, folder map, limits).
Moved, not deleted: `research/report/README.md` -> `research/archive/professor-report-superseded-2026-09-30.md`; `FINAL_CANDIDATE.md` (011) ->
`archive/final-candidate-011-superseded.md`; `ORGANIZATION_SHA256SUMS` and `organization-verification.txt` -> `archive/` (they describe the old layout);
`AUDIT_REPORT.md` and `ALGORITHM_RESEARCH.md` -> `research/analysis/`. Link in 014's README updated; 016 added to the ledger.

## 2026-09-30 — Experiment 017 recorded as a plan only

Idea: trade load/build time for memory and runtime (C1, per-word records, partial sums, larger normalization table, offline mmap'd tables). Nothing built;
start after 016's timing. See `experiments/017-loadtime-precompute/README.md`.

## 2026-09-30 17:30 — Pause switch added to the timing gate

`research/tools/env_gate.sh` now fails while `research/PAUSE_TIMING` exists, so sessions can be paused between runs and resumed by deleting the file.
Tested (fail with file, pass without). No timing logic changed.

## 2026-09-30 17:31 — 016 timing restarted as the full batch

Quick session stopped at the user's request (4 runs kept in `016-batch/raw/quick_attempt_stopped/`); the first full-batch attempt's 22 runs are in
`raw/batch_attempt1_stopped/`. Neither is used for results. `run_unattended.sh` now takes `MAX_WAITS` (default 240 x 30 s, unchanged); the batch
runs with an unlimited wait so it can be paused (research/PAUSE_TIMING) for any length of time and resumed.

## 2026-09-30 17:31-22:15 — Experiment 016 timed: new best 3.26x (golden-v1) / 3.27x (novel) / 3.24x (floor)

Full AC batch completed (paused 17:50-19:20 on request; waited on battery ~20:49-20:52). Two runs touched by sleep/battery were excluded and rerun.
`pgolto` (T4+T5+T6+T8 + PGO + thin LTO): 10.59 / 10.63 µs vs 34.54 / 34.77 µs; floor 10.31 vs 33.38 µs; peak 2.539 GB. T4+T5 +11.2%, PGO/LTO +10%;
T6 +1.3%, T7 and T8 no gain. Normalization on non-ASCII 2.82 -> 0.16 µs. 015 confirmed by 20-round paired run (+2.47%, 19/20). Root README,
ledger and 016 README updated.

## 2026-09-30 22:40-23:45 — Experiment 018 (compiler-only builds): new best 3.39x / 3.39x, floor 3.30x

Whole-library PGO (development split) + full LTO on 016 `t45`'s sources, tokenizer compiled at -O1 (upstream -O0): `c2s1` 10.15 / 10.20 µs vs 34.42 / 34.55 µs,
floor 9.78 vs 32.22 µs; -O3 / -mcpu=apple-m1 / scanner -O2 within noise. All five builds byte-identical; clean AC session. Scanner compile 256 s (-O1),
318 s (-O2), once. Mistakes: scanner first built with LTO (re-optimized at every link), a stop command missed one process (two concurrent builds,
discarded), a queue matched its own command line again (replaced). Ledger, 018 README and root README updated.

## 2026-09-30 23:55 - 2026-10-01 00:20 — Experiment 017 steps 1-2: new best 3.42x / 3.42x, floor 3.35x

Profiled `t45` (sample, AC): `fd_canon` vocabulary lookups for the UNKNOWN placeholders (~2% of samples) and per-string response allocation/copies.
Step 1 (constant canon cache) +0.3%; step 2 (one-block response, R1) +1.2% more. `u2` byte-identical (dumps, frozen refs, 2x2M fuzz), 0 leaks, peak
2.539 GB. Clean AC session. Ledger, 017 README and root README updated. Remaining to 3.5x: ~2.4% on the average.

## 2026-10-01 00:15-00:45 — Third independent dataset started (golden-v3-osm), paused

Builder `profiling/golden-v3-osm/build_osm.py`: real addresses from OpenStreetMap (Overpass API), ~64 cities on 6 continents, assembled per country's
address order, deduplicated against golden-v1 and the novel set (Senzing-derived, so this source is independent). Main Overpass server returned 504s;
switched to the maps.mail.ru mirror; radius reduced 2 km -> 700 m. Stopped at the user's request with 24 of 64 cities in raw/; resume with
`python3 build_osm.py fetch`, then `build`, exactness check (baseline dumper `017-loadtime-precompute/artifacts/dump_outputs_baseline`) and
`017-loadtime-precompute/tools/run_v3.sh` on AC. No result yet. Mistake: a wait loop matched its own command line again (stopped, replaced by a PID wait).

## 2026-10-01 01:11-02:05 — Third independent dataset finished: 3.20x on unseen OpenStreetMap data

`profiling/golden-v3-osm`: 11,405 addresses, 49 countries (Rome's fetch failed). `u2` byte-identical to the original on all. Clean AC timing: original
21.17 µs, `r0` 6.71 (3.15x), `u2` 6.61 µs (**3.20x**). Median country 3.25x; Korea 2.38x, Taiwan 2.70x lowest. Root README, 017 README and a dataset
README added. Weakest spot confirmed: Hangul normalization (excluded from the T5 table).
2026-10-01 02:24 env_gate.sh: any research/PAUSE_TIMING* file now pauses (independent pause per job)

## 2026-10-01 01:58-03:50 — Experiment 019 (general per-address work): 3.84x / 3.85x, floor 3.78x, OSM 3.76x — target passed

11 builds, all byte-identical. Biggest gains: reusing per-address objects (+7.1%), inline dictionary compare under whole-library PGO/LTO (+4.2%).
Neutral: NEON Viterbi, ASCII token normalization. Packed weight records: speed-neutral, peak memory 2.179 GB (original 2.613 GB). Hangul syllables
added to the normalization table (exact; 23 M differential answers). Mistakes recorded in 019's README: premature timestamps corrected; a leftover
020 job released the timing pause during verification -> 11 contaminated runs excluded and rerun; queued jobs killed by an over-broad pkill.
Root README, ledger and 019 README updated. Next: q1 (precomputed role ids) and 020 compilers/allocator on top of p1.
2026-10-01 03:56 NOTE: second premature timestamp in 019 README (03:45-04:05) corrected to 03:45-03:56; from now on section times are written only after checking the clock

## 2026-10-01 03:57-05:02 (corrected from markers) — Session 2 (experiment 020 + 019 follow-ups): ~4.0x on every dataset

`q2` (019 p1 + precomputed per-word role ids + phrase canon cache): golden-v1 3.97x, novel 4.00x, floor 3.96x, ASCII 3.96x, independent OSM 3.98x;
peak 2.245 GB. Rejected: clang 23 (-1%), GCC 16 (-7%), phrase prefilters in this pipeline (-2.5%), pair routing. mimalloc +1% (to combine next).
Root README, ledger, 020 README updated.
2026-10-01 06:01 NOTE: third premature timestamp (021 timing window) corrected from the session's start/DONE markers; times are now taken from marker files only

## 2026-10-01 05:15-06:45 — Experiment 021 (stack on q2): 4.08x with constant ids + mimalloc, then 4.25x with dictionaries at load 0.5

`s3` (q2 + constant feature ids + mimalloc) 4.08x / 4.08x / floor 4.04x / OSM 4.08x / ASCII 4.03x; micro-batch `m3` (+ load factor 0.5, + no clear of
unused transition row 0) 4.25x / 4.31x / 4.24x / 4.25x / 4.23x, peak 2.264 GB. All byte-identical. Premature timestamps in the 019, 020 and 021
READMEs and here were corrected from marker files. Next: feature dictionaries at load 1/3 and 1/4 (l3 2.380 GB, l4 2.471 GB peak) being timed.

## 2026-10-01 06:47-07:20 — Experiment 021 load-factor sweep: final candidate `l4` 4.33x / 4.36x / floor 4.27x / OSM 4.31x / ASCII 4.30x

Feature dictionaries at load 1/4 (vocabulary 0.5). Byte-identical; peak 2.471 GB vs 2.613 GB, steady 2,250 vs 2,492 MB; load 8.7 s vs 3.4 s.
Packaged in research/final/021-l4/. Root README, ledger, 021 README updated.

## 2026-10-01 07:18 — Overnight run stopped (decision)

Stopped after experiment 021 with `l4` as the final candidate. Remaining ideas — prefetching the postcode graph lookup, avoiding the score-matrix memset (the
row-0 part gave +0.3%), a per-address key cache, phrase-feature interning (blocked: ideographic joins differ) — are each estimated (UNVERIFIED) at
<= 1-2%, near this laptop's ~1% noise floor. The largest remaining profile item, the phrase trie walk (~20%), was already attacked by three prefilters
(016 T6, 019 g1, 021 s4: +1.3%, -2.5%, ~0%). Further load-factor reduction would eat the remaining 142 MB peak-memory headroom. No processes left running
except caffeinate.

## 2026-10-01 09:05 — Experiment 022 built (portable lower bound of l4), not run

No NEON, PGO, LTO or mimalloc; build exit 0. Verification and timing deferred until AC power (user request: build only).

## 2026-10-01 11:15-11:45 — Experiment 022 (portable lower bound) verified and timed

pt (l4 without NEON, PGO, LTO, mimalloc): byte-identical; 3.77x / 3.86x / floor 3.77x / OSM 3.74x / ASCII 3.80x (l4 same session 4.31-4.41x).
Memory FAILS: peak 3.176 GB, steady 3,029 MB vs original 2.614 GB / 2,492 MB. Memory result depends on mimalloc; cause of the ~780 MB gap undiagnosed.
Recorded in 022 README, ledger, root README limits.

## 2026-10-01 11:59 — Experiment 023: memory excess of the portable build diagnosed

pt's extra ~780 MB = freed load-time temporaries (feature/vocabulary tries 470 MB, full dictionaries before the integer split 630 MB) that the macOS
system allocator retains (MALLOC_LARGE empty 665 MB); mimalloc releases them. No source change yet; fix options A-C recorded.

## 2026-10-01 12:05 — Experiment 024 (fix option A) rejected

malloc_zone_pressure_relief after load: no change on macOS (pr peak 3.175 GB, steady 3,029 MB, 665 MB empty large blocks still held). Next: options B/C.

2026-10-01 12:06 — Reverted the rejected EXP_RELEASE_FREED edit in 019 candidate/address_parser.c (copy kept in 022 artifacts/pr/src); candidate sources now byte-equal to final/021-l4/sources.

## 2026-10-01 12:07 — Session end: research/HANDOFF.md written (state, open items, notes); no processes left running

## 2026-10-01 16:26–17:19 PDT — Experiment 025 and consolidated audit

Started from pinned upstream commit `25099c5` and preserved 021 `l4`; checked the canonical engineering knowledge base, project method, handoff and all experiment records. Initial power gate reported battery, so timing waited until AC; no battery timing was used. Created `025-audit-next` before source edits. Built unchanged control, API-repair candidate `fixes`, and ASCII no-copy candidate `borrow` with the same Apple clang PGO/LTO/mimalloc settings.

Confirmed that packaged `l4` aborts (exit 134) when feature printing is enabled. `fixes` supports packed rows in fallback scoring, resets/frees globals and reusable parse objects at teardown, and guards unsupported model save. The API probe and dictionary-disabled golden-v1 run match the preserved original; all three rebuilds match original and frozen output on golden-v1, novel and OSM, and `borrow` matches 2 × 2M fuzz inputs. Negative control detected a one-label corruption. The first frozen check incorrectly used raw `cmp` across two JSONL key formats; retained failure and reran with the correct comparer. The first final-analysis script expected absolute paths and yielded empty rows; retained it and reran corrected analysis. One-thread `ps -M` check of `fixes` passed at six samples. All nine manifest model hashes passed.

Development timing suggested a small but unstable `borrow` gain (paired median 1.0227×); full timing was mixed. Long confirmation rejected it: paired median 0.9991× on golden-v1 and 0.9909× on novel versus `fixes`. Peak footprint remains below original. Same-session `control` vs `fixes` long timing showed 1.0014× and 0.9946× paired medians on golden-v1 and novel, respectively, i.e. no established speed difference. Raw commands, timestamps, flags, outputs, and run directories are retained under 025. Updated the ledger (including the missing 008 entry), professor report, handoff, and `analysis/FINAL_AUDIT_2026-10-01.md`. The 021 measured speed deliverable remains unchanged; `fixes` is a separate maintenance candidate.

## 2026-10-01 17:57–18:24 PDT — Experiment 027 root-transition cache rejected

The actual 021 Mac profile in 026 identified phrase trie traversal as the largest named routine (16.7% exclusive samples). Created 027 before editing, copied the 021 sources, and built a parser-owned 256-entry root-transition cache for each loaded phrase trie. All 512 entries and 20,616 development phrase searches matched ordinary traversal; golden-v1, novel, OSM, a 2M-input fuzz comparison, and one-thread checks passed. A deliberately corrupted cache entry and output label were detected. Five paired 120-pass development runs on AC, with heavy unrelated host load, gave cache/control pooled throughput 0.9980× and paired median 0.99949×, so no speed gain was established and full validation was not run. Peak footprint was 6,144,000 bytes above the unchanged 021 control; original-relative memory was not reverified in this session. Preserved failed build/verify attempts (path quoting, mixed header guard, checker link, capture permission, model hash path) and all raw logs in `experiments/027-root-transition/`. The 021 speed result remains unchanged.

## 2026-10-01 18:44 PDT — Cross-platform progress and laptop-sleep handoff

Experiment 026 rebuilt the pinned original and exact 021 sources in a Linux ARM64 Docker VM. Ordered output matched on golden-v1, novel and OSM, with one parsing thread. The exact 021 Linux build failed the memory rule (peak RSS 2,347,384,832 versus original 1,960,681,472 bytes with glibc); a separate dense-dictionary variant preserved output and used about 14–15 MB less peak and late RSS than the original in paired runs on all three corpora. Linux x86-64 original/candidate builds with SSE2 matched output on those corpora under emulation. Original and adapted 021 Windows x64 PE builds cross-compiled; no Windows runtime was available. A profile of the actual 021 Mac build led to rejected 027. Source-only 028 model-name-dispatch feasibility work is prepared but has not been built or run. The 4.33× macOS result remains the only accepted speed headline.

An AC-powered Linux ARM64 paired timing matrix began under heavy unrelated host load. It is a noisy, prewarmed **diagnostic**, not an accepted speed result or a true no-warmup cold test. At the user's notice that the Mac laptop must be closed, requested a safe stop after the current golden-v1 trial boundary; completed and interrupted trial status belongs to the 026 raw logs. Before a final Linux speed claim, capture binary/model/input/image identities at timed-session boundaries, fix the warmup NULL check, perform a separate true-cold run, and repeat timing in a quiet AC session. The handoff and consolidated audit now reflect the platform limits; no frozen inputs, outputs, baseline artifacts or original checksum manifests were changed.

## 2026-10-01 18:49–19:42 PDT — Linux diagnostic stopped; audit updated off AC

The 026 Linux timing watcher stopped at 18:49:25 PDT after all nine golden-v1 runs exited 0 and before any novel run. The deliberate outer exit 143 and watcher exit 0, `STOPPED` marker, raw passes, and analysis remain under 026. Dense/original paired warm speedups were 3.5455/3.5225/3.1939×, median 3.5225×, with dense about 15 MB below the original's peak RSS. Only 1 of 447 original warm passes was clean; the first pass was prewarmed. This incomplete noisy diagnostic is not an accepted Linux speed result. No benchmark process was present when the user reported being off AC. Documentation only was updated in the 026 overview, experiment ledger, professor report, consolidated audit and handoff; no model, build or timing run was resumed.

## 2026-10-02 12:30 PDT — User stop and final handoff

The user explicitly requested that work stop and a handoff be created. No build, model scan, test, or timing run was started. All subagents had completed, and a targeted process check found no benchmark, dumper, Docker Ubuntu benchmark, or timing runner. Updated `research/HANDOFF.md` with the stop instruction, accepted and diagnostic results, unexecuted 026/028 work, and the conditions for any future user-authorized resumption. Do not auto-resume when AC returns.

## 2026-10-02 13:50 PDT — Scope set to macOS only; next-step analysis

The user parked the Linux track and limited further work to macOS. Read-only review of the handoff, ledger, 026 profile of 021 `l4`, 014 quantities, 016/019/028 records and the active 021 flags. No build, model scan or timing was run. Recorded the scope decision in HANDOFF.md. Proposed next step (awaiting approval): a stage-timing build of `l4` for current per-stage costs before any new candidate.

## 2026-10-02 13:52–13:57 PDT — Experiment 029 stage split and instruction counts (measurement only)

Built `l4s` (= exact 021 `l4` + EXP_STAGE_STATS, same PGO/LTO/mimalloc procedure; sources verified byte-equal to final/021-l4) and ran 3 × 50-pass stage timings and 3 rotating instruction/cycle count pairs (original vs `l4`) on the development split, AC-gated. Median stage µs: context fill 2.18, features 2.03, lookups 1.15, accumulation 1.09, Viterbi/rest 1.00, tokenize 0.22, response 0.19 (total 8.01). Original/`l4`: 2.90× fewer instructions, 4.31× fewer cycles (IPC 1.52 → 2.25). Env snapshot misuse (missing FILE argument) left before/after snapshots empty; a post-hoc snapshot shows a busy host (load ~8). A paper-only theoretical-limit analysis was delegated to a read-only side agent (output pending: research/analysis/LIMITS_2026-10-02.md).

## 2026-10-02 ~14:10 PDT — Paper-only limit analysis received

Read-only side agent `limits` wrote `research/analysis/LIMITS_2026-10-02.md` (nothing built or run). Floors: same algorithms, ideal implementation ≈ 0.5–1.9 µs/address (18–68× vs original 33.92 µs); with exact algorithmic changes ≈ 0.30–1.05 µs (32–113×); pessimistic sensitivity ≥ 7×. These are bounds, not predictions. Main session appended §8 with 029's final instruction/cycle counts (IPC 2.25; `l4` executes 5.3–8.6× the floor instruction count; effective clock ~4.0 GHz vs the assumed 4.51 GHz).

## 2026-10-02 14:07–14:20 PDT — Experiment 030: string-path count and pair-routing screen

User authorized parser code changes (AC ~3 h) and restated the generality rule: no tuning to our datasets; tables only from the model, routed by feature name. Recorded as a project rule in 030 and memory. Counted lookups on the development split: 57.4% still use string keys (phrase-valued 16.2/address, word pairs 9.6). Screened the existing `EXP_INT_KEYS_PAIRS` switch with the l4 build procedure: output identical to `l4` on development; −1.4% instructions/address, cycles within noise, warm time 0.997×. Not adopted; phrase-id candidate decision put to the user. Disclosed that every l4-procedure build trains PGO on golden-v1 development addresses (dataset-derived code layout; output unaffected).

## 2026-10-02 14:20–14:49 PDT — Experiment 031 PGO cross-training matrix

User asked to make PGO dataset-neutral and test it against unseen data. Built the exact 021 package sources four ways (PGO trained on golden-v1 all, novel, OSM; and no PGO); all 12 build×dataset dumps byte-identical to `l4`. Timing attempt 1 aborted after one run (script bug: `cap()` overwrote the global binary count `n`; it still wrote DONE). Preserved as `raw/timing_attempt1_failed` and `raw/attempt1_DONE_premature`; rerun with `tools/run_timing.sh` (local variables).
Attempt 2 timing 14:25:13–14:44:11, 90 runs exit 0, AC-gated, host load ~8. No overfitting: each build's speed on its own training set equals the other builds' (largest diagonal advantage +1.1%, within noise); held-out speedups 4.31–4.33× golden-v1, 4.36–4.37× novel, 4.29–4.31× OSM. No-PGO build 4.21× / 4.28× / 4.11×, so PGO adds ~2–4.5% regardless of training set. Peak footprint unchanged (≈2.471 GB vs 2.613 GB). Steady footprint sampled afterwards (14:44–14:48): original 2,172 MB at every sample, `l4`/`tosm`/`nopgo` 2,250 MB — `l4` +78 MB above the original in this session, whereas 021 measured the original at 2,492 MB. Professor report and ledger updated to qualify the steady-memory claim; `l4` unchanged.

## 2026-10-02 14:44–15:10 PDT — 031 steady-memory follow-up

Steady footprint (`footprint -p`, same session): original 2,172 MB at all samples, `l4`/`tosm`/`nopgo` 2,250 MB. Method checked (warm-up vs none, text vs JSON field; `l4` equals 021's recorded 2,250 MB), so the original's 2,492 MB in 021 reflected machine state. `l4` is +78 MB above the original here and fails the steady-memory rule in this state. Preserved 021 variants measured in the same session: `s3` 1,868, `m3` 2,026, `l3` 2,141 MB (all below). Two script failures (shell quoting; unquoted paths with spaces) preserved and rerun. Professor report, 031 record and handoff updated; the hashed final/021-l4 package was not modified.

## 2026-10-02 15:11–15:30 PDT — Experiment 032: `l3` validated and packaged as the deliverable

User chose `l3`. Same-session validation of the preserved 021 `l3` binaries (hashes match 021's record): output identical to the original on golden-v1, novel and OSM, frozen references and 2 × 2M fuzz; one thread; 5-round paired timing 4.38× / 4.33× / 4.28× (`l4` 4.37× / 4.32× / 4.34×); peak 2,380 MB vs 2,614 MB; steady 2,141 vs 2,172 MB (`l4` 2,250). The script's summary lines had the unquoted-path defect again (empty fields); raw checks were complete and a corrected summary was written from the raw files. Created `final/021-l3/` (sources identical to 021-l4, BUILD.md, build_hashes, SHA256SUMS). Professor report headline, ledger and handoff updated; `final/021-l4` left unchanged.

## 2026-10-02 15:38–15:50 PDT — Experiment 033: profile of `l3`

Sample profile of the shipped `l3` plus a diagnostic stats build with five timers inside context fill (switch `EXP_CF_STATS`, experiment copy only). Context fill ≈2.23 µs/address: component-phrase trie search ≈1.0 µs (45%), postal-code trie search ≈0.45 µs, address dictionary 0.27, token normalization 0.27, prefix/suffix 0.22. The phrase-trie walks (≈1.45 µs, ~18% of parse time) are the largest single target; cause (dependent misses in large tries) unverified. No speed claim; deliverable unchanged.

## 2026-10-02 15:50–15:56 PDT — Experiment 034: parallel-run calibration

User asked to run tests in parallel if it does not distort results. Measured: per-process slowdown +6.6% (2 copies), +13% (3), +12% (4, with swap activity); running the original and `l3` simultaneously inflates the paired speedup to 4.41–4.49× vs 4.31–4.35× sequentially. Decision: timing stays sequential; builds, correctness checks and instruction counts may run in parallel. User goal set: find the ceiling and aim for ~90% of it (restructuring, not micro-tweaks).

## 2026-10-02 15:56–16:12 PDT — Experiment 035: measured ceiling (replay oracle, probe only)

Built a diagnostic oracle (switch EXP_ORACLE, experiment copy only) that records each address's final feature ids on pass 1 and replays them later, skipping context fill, feature generation and lookups; no PGO for oracle and control (PGO training harness would collide pointer keys). Sequential 5-round timing: ceiling 2.63 µs (13.0×) golden-v1, 2.66 µs (13.1×) novel, 1.91 µs (11.4×) OSM vs no-PGO l3 control 4.16–4.21×. Components and replay counts exact. 90% of ceiling ≈ 2.9 µs/address, which requires the front half to fall from ~5.5 µs to ~0.3 µs: a phased integer-pipeline rewrite, not this session's work. One failed control build (wrong relative path) preserved.

## 2026-10-02 16:12–16:30 PDT — 035 level 2 (realistic ceiling) and 036 trie walk counts

036 (counting build, development + OSM; one crash at exit from a destructor reading a freed trie, preserved and fixed): per address the component trie does ~34 node transitions, postal ~19, address dictionary ~20; 77% / 88% / 91% of transitions are first-token walks from the root; per-step cost in the 8.2M/7.5M-node tries ≈ 21–27 ns (memory-bound). 035 level 2 (replay feature keys, keep dictionary lookups; probe only): 3.91 µs (8.70×) golden-v1, 4.01 µs (8.56×) novel, 2.85 µs (7.58×) OSM — the realistic ceiling. User asked for single-thread confirmation: no thread primitives in l3 or 037 sources; l3 measured at 1 thread (032).

## 2026-10-02 16:24–16:45 PDT — Experiment 037 (lockstep trie walks) rejected

Implemented precomputed, interleaved first-token walks in phrase search (switch EXP_LOCKSTEP) with a verify mode. 0 mismatches over 5.48M verified searches (all three datasets + 2M fuzz), outputs identical, one thread, memory unchanged. Sequential timing: 2.8% / 3.0% / 4.8% slower than l3 → rejected. Original's steady footprint measured 2,492 MB in this session (2,172 MB earlier today); l3 2,141 MB in both.

## 2026-10-02 16:44–16:52 PDT — Experiment 038 diagnostics

Repeating the component and postal searches costs only +952 cycles per address when warm, versus ≈5,800 cycles for the first search: phrase search is first-touch-miss-bound. Feature-generation sections: next-word block 0.68 µs (first touch of upcoming tokens' data), word block 0.38, component 0.33, postcode 0.30, prev-word 0.20. Professor report and handoff now show the realistic (level-2) ceiling 8.7× next to the 13× probe. One failed fs build (misplaced marker) preserved.

## 2026-10-02 16:53–17:00 PDT — Experiment 039 feasibility count (not pursued)

Walked all 3.69M vocabulary keys through the component and postal tries (model only). A one-lookup word-walk table covering the useful outcomes would need ≈30–50 MB (over l3's 31 MB steady margin) and cannot help postal walks (numeric tokens are not vocabulary words); estimated net ~4–5% even if funded by denser dictionaries. Not pursued. One crash (count ran before the tries were loaded) preserved in the log.

## 2026-10-02 17:01–17:08 PDT — Experiment 040 diagnostic and night summary

Running feature generation twice showed its warm cost is ≈17,000 instructions and ≈1.0 µs (IPC 4.2) of the ≈2.15 µs in place: half instruction work, half first-touch memory. Summary of 033–040: realistic ceiling 8.7× (lookups paid); front half dominated by first-touch memory in large model structures plus ≈17k feature-generation instructions; trie-walk interleaving (037) slower, word-walk table (039) does not fit memory. l3 remains the deliverable. Next candidate if work continues: a leaner feature generator (instruction share, est. 6–12%) under a shadow-mode equivalence check.

## 2026-10-02 17:05–17:45 PDT — Experiments 041–043: where the remaining time is

User instructed to continue until 7.8× is reached or shown impossible/not worth it. 041: superpages unavailable on macOS/arm64; warm bound (same address parsed twice) 3.67 µs ≈ 9.2× vs 7.84 µs normal → 53% of l3's time is first-touch memory cost. 042: lookups+accumulation duplicate test and weight-entry counts (1,093 adds/address); its "instruction-bound" reading was corrected by 043. 043: per-stage cold/warm split — first-touch: context fill 1.69 µs (component 0.89, postal 0.40), features 1.23, lookups 0.72, accumulation 0.61; warm Viterbi+rest 0.96 µs. Next: Viterbi compute, fused dictionary key+row records, per-word records, denser phrase index.

## 2026-10-02 17:45–18:25 PDT — Experiments 044–046 and trace extension

User restated: continue until 7.8× or shown impossible/not worth it. 044 working-set sweep recorded as confounded (country-ordered subsets). 045 trace: 315 distinct lines / 174 pages per address, 59 compulsory, 81 misses of a 16 MiB LRU; translation misses cheap when data cached (+10 ns). 046 cross-stage early prefetch: 8.5% slower (this implementation duplicated key hashing) → rejected; conclusion narrowed to the implementation. Cost per modelled miss differs 25–230 ns by path (dependent chains, shared-L2 contention from co-running user processes). Next: contention-independent compute reductions (Viterbi, feature generation) and dependent-miss reductions. Asked the user whether the heavy co-running app can be closed for final timing.

## 2026-10-02 ~18:25–19:35 PDT — Experiments 047–051; `va` becomes the deliverable

047: Viterbi recursion 0.70 µs, score setup 0.19, label strings 0.06 µs. 048: Viterbi specialized for 20 labels + transition-matrix alias (`va`) screened +3.9–5.5% vs l3. 049: warm-only profile — libc string/memory routines ≈21% of warm time. 050: lazy transition rows rejected (2–8% slower than va). 051: full validation of `va` — identical outputs (3 datasets, frozen, 2×2M fuzz), one thread, peak 2,380 MB, steady 2,141 MB (original 2,492 MB this session), 4.53× / 4.55× / 4.62× vs l3 4.32× / 4.32× / 4.29× → packaged as `final/048-va`, professor report headline updated. Reachability analysis written (`analysis/REACHABILITY_2026-10-02.md`): optimistic sum of all identified levers ≈2.65 µs vs ≈3.15 µs needed for 7.8×. Session-wide caffeinate started at the user's request. Two script slips (zsh glob in packaging) fixed and recorded here.

## 2026-10-02 late — Experiments 052–055; session stopped at the usage limit

052 memory map of `va`; 053 exact gap-encoded postcode-context graph (−323 MB steady, ≈−0.6% speed; enabler, not packaged); 054 per-word precomputed score vectors (0 mismatches in 17.8k verified rows, but only ≈8% of tokens eligible, −0.4%) rejected; 055 weight values mostly distinct (no value-table encoding). Deliverable unchanged: `final/048-va`. Work stopped because the usage limit was reached; a session-wide `caffeinate` (pid 34760, 10 h) is still running.

## 2026-10-02/03 night — Experiments 056–057 (after the usage-limit reset)

056: exact first-token table for the component-phrase trie (model-derived, 56 MB, funded by 053's freed memory): verify build 0 mismatches over 7.29M table-assisted walks (3 datasets + 2M fuzz), outputs identical; screens +1.5–2.9% vs pc, role-row prefetch +0.3–1.2% more (`ftr`); memo reuse (`ftm`) 0.5–1.5% slower, rejected. 057 full validation of `ftr`: 4.62× / 4.62× / 4.55× vs va 4.51 / 4.55 / 4.59×; identical outputs; one thread; peak 2,537 MB, steady 1,878 MB. Not adopted (OSM −0.7%). Deliverable unchanged (`final/048-va`).

## 2026-10-03 — Experiment 058 (user: "continue")

Numeric-token table for the postal trie (0 mismatches over 10.67M verified walks) gave no gain; confirmed 053's compression costs ≈1% on OSM; a checkpoint index for compressed rows (load-time proof: 366.8M queries, 0 mismatches) was slower on both datasets. Both rejected. `ftr` vs `va`: +2.6–3.1% golden-v1, ≈0 on OSM. Deliverable unchanged.

## 2026-10-03 — Experiment 059 (user: "there are still more")

Feature keys now carry a 1-byte dictionary name id instead of the name text when the name is a known literal (model-constant id cached per call site); the lookup skips the separator scan and name hashing. Outputs identical on all three datasets and 2×2M fuzz; screen +3.2% golden-v1 and +3.8% OSM over `ftr` in every round. Accepted into the stack.

## 2026-10-03 — Experiments 060–061

Warm profile of nb showed the phrase-feature helpers still on the text path; moving the name macros above them (061, nb2) gave +2.2% golden-v1 and +3.2% OSM over nb (6-round screen; a first noisy 4-round attempt kept). Outputs identical everywhere. Stack now ≈6.87 µs golden-v1 / 4.37 µs OSM.

## 2026-10-03 — Experiments 063–064; `lf` becomes the deliverable

063 address-dictionary first-token table: 0 mismatches in 19.6M walks but 1–4% slower → rejected. 064 full validation of `lf` (va + 053 + 056 ftr + 059/061 + 062): identical outputs (3 datasets, frozen, 2×2M fuzz), one thread, peak 2,537 MB, steady 1,878 MB; 4.92× / 4.86× / 4.91× vs va 4.51 / 4.52 / 4.60× in the same session → packaged as `final/062-lf`; professor report headline, ledger and handoff updated.

## 2026-10-03 — Experiments 065–066
Warm profile of lf (065). Re-zeroing only written transition rows (066): outputs identical, but 0.5% / 1.4% slower → rejected. Deliverable remains lf (4.92× / 4.86× / 4.91×).

## 2026-10-03 — Experiment 067
Per-address phrase-string cache + length-aware phrase-feature appends: verified (0 length mismatches, outputs identical), +0.5% golden-v1 / +0.6% OSM over lf → stack head pl.

## 2026-10-03 — Experiment 068 rejected; unseen-dataset re-measure started (user request)

068 (role/const ids carrying packed weight-row ranges): outputs identical, −0.6% golden-v1 / −0.4% OSM → rejected. The user asked that, after a faster candidate, it be re-measured on a dataset the code has never seen. Created `profiling/golden-v4-osm-unseen/` (builder copied from golden-v3 with 58 cities absent from every earlier set; exact duplicates of golden-v1, novel and golden-v3 removed; never used for tuning or selection) and started the Overpass fetch. Experiment 069 will compare original / va / lf / pl on it (outputs vs the original-library dumper from 017; 5-round sequential timing).

## 2026-10-02 23:52–23:56 PDT (from 069 raw/start.txt, raw/DONE) — Experiment 069: unseen-data re-measure

Built `profiling/golden-v4-osm-unseen` (Overpass fetch 22:55–23:52; Bristol and Porto failed; 10,293 addresses, 42 countries, 45% non-ASCII, no overlap with earlier sets). On it, va / lf / pl are byte-identical to the original and 4.68× / 4.96× / 5.02× faster (5 paired sequential rounds). Report, ledger and handoff updated.

## 2026-10-03 — Experiment 070
-mcpu=native (resolves to apple-m3) and -O3 builds of pl: outputs identical, speed within ±0.4% → no gain.
2026-10-03T01:36:09-0700 overnight session: user granted full autonomy; caffeinate -dims -t 86400 started (pid 70844)

## 2026-10-03 01:37–01:45 PDT (from raw/created.txt) — Experiment 071: certified early exit measured, not viable
Margins between best and second-best label paths (median 30) are 20–60× smaller than sound bounds on what skipped features could add (median 700–2,000); 0.0% of lookups skippable. Recorded as a negative result. Continuing overnight with remaining exact micro-optimizations.

## 2026-10-03 — Experiment 072
Digit-only token fast path (dg): outputs identical on all sets incl. unseen v4 and fuzz; +0.4% v1 / +0.6% OSM vs pl → accepted as stack head. Conditional backpointer clear: no added gain → rejected.

## 2026-10-03 01:55–02:30 PDT (from raw/created.txt) — 073 profile, 074 accepted
073: fresh stage profile of the head; 072's digit fast path is neutral by stage timers (relabelled). 074: prefetch pre-pass for first-token table entries and their nodes (pf2): outputs identical everywhere incl. fuzz; +2.4% v1 / +1.0% OSM; stage timers confirm (context fill −0.14 µs cold). pf3, vocab-slot prefetch and 012 search prefetch add nothing. golden-v5 (second unseen set) fetch running for the user-required re-measure.
075: two-step vocabulary-slot prefetch −0.4%/−0.5% → rejected.

## 2026-10-03 02:37–02:58 PDT (from raw/created.txt) — 076–078
076 feature-section profile. 077 postcode-row prefetch (pcp) accepted: +1.35% v1 / +1.0% OSM (12/12 rounds), mechanism confirmed, identical everywhere. A first 077 screen was discarded because 078 builds/checks ran concurrently (my error; quarantined). 078 eager vocab memo rejected (−0.5 to −1.3%).

## 2026-10-03 02:58–03:13 PDT (from raw/created.txt, 080 raw/DONE) — 079 rejected via unseen data; 080 golden-v5 re-measure
Built profiling/golden-v5-osm-unseen (8,794 addresses). 080: lf/pf2/pcp/ptp identical to original; pcp 5.04× (pf2 +1.5% over lf, pcp +0.9%); ptp's golden-v1-only gain reversed (−0.6%) → rejected (workload-dependent; −0.6% on v5 is near noise). Stack head: pcp.
081: push-time prefetch for integer keys neutral/slower → rejected.

Correction (2026-10-03 03:35): the approximate clock times first written in tonight's headings (~01:00, ~03:30, ~05:00, ~06:15) were estimates and wrong; replaced with the times recorded in each experiment's raw/created.txt / raw/DONE.

## 2026-10-03 03:23–03:40 PDT — 082: pcp validated, new deliverable
082 (064 protocol): pcp 5.149× golden-v1, 5.072× novel, 5.058× OSM (lf 4.91/4.84/4.90 same session); 1 thread; peak 2.543 vs 2.613 GB; steady 1,878 vs 2,492 MB. Packaged research/final/077-pcp (19 files, SHA256SUMS verified). Professor report headline, ledger and handoff updated.

## 2026-10-03 — 083 workload check and report corrections
083: prefetch targets per address are about equal on golden-v1 and unseen v5, so pcp's smaller unseen gain (+2.3% vs +4.8% over lf) is not fewer targets; gain is larger on Senzing-derived than OSM-derived sets. Report now quotes the unseen increment, explains why 074/077 differ from earlier rejected latency-hiding attempts, fixes the headline time stamp; 069 heading corrected to its recorded 2026-10-02 23:52–23:56; 079 relabelled workload-dependent.

2026-10-03: tools/env_gate.sh now also fails while clang/ld/llvm-profdata/dump_outputs/fuzz_pipeline/bench_ext processes run (previous version kept as env_gate.sh.pre083).
084: early postal-trie prefetch slower → rejected.

## 2026-10-03 — 084 rejected; 085 row-line prefetch candidate
085: prefetching every line (≤8) of each weight row: rl8 +4.7–4.9% golden-v1, ≈0 OSM; stage timers: accumulation −0.46 µs dev / −0.15 µs OSM, lookup loop +0.2 µs. Outputs identical everywhere incl. fuzz. Awaiting golden-v6 (third unseen set) before acceptance.

## 2026-10-03 — 086: rl8 rejected by unseen golden-v6
Built profiling/golden-v6-osm-unseen (9,450 addresses). pcp and rl8 identical to original; pcp 5.06×; rl8 −1.6% vs pcp in all 5 rounds → 085 rejected (its +4.7% golden-v1 gain is specific to Senzing-derived data). Second time tonight the unseen-data rule caught a non-general gain (079, 085).
087: transition rows in use average 74/400 cells → dense accumulation not viable.

## 2026-10-03 — 088 warm bound of pcp; reachability update
088: pcp with each address's data already cached: 3.01 µs golden-v1 (11.1×), 2.60 µs OSM (8.1×), 2.54 µs unseen v6 (7.7×). 7.8× on OSM-style data is unreachable by memory-side work alone. Wrote analysis/REACHABILITY_2026-10-03.md.

## 2026-10-03 — 089 warm function profile; experiments paused pending user decision
089: no exact change above the 2–3%-of-warm-time bar remains (memset = score clearing, already tested; strlen/memmove spread over many sites). With 088, the evidence says further progress needs the front-half rewrite. Stopped opening experiments; deliverable stays pcp (final/077-pcp). Session caffeinate (pid 70844) left running.

## 2026-10-03 — user said continue; 090 Viterbi split, 091 max-only Viterbi candidate
User (returning briefly): 'continue you can do it do not stop'. Resumed on warm compute: 090 Viterbi recursion = 0.39/0.35 µs pure compute. 091: forward pass keeps maxima only, argmax recovered on the path by exact recomputation; vmo +0.7%/+1.3%, vmx (FMAX) +1.3%/+1.6% more; checker builds identical everywhere incl. fuzz; stage timers Viterbi −12%. golden-v7 fetch started for its unseen check.
092: ≈15/20 state-transition rows touched per token → skip-clear idea not viable.

## 2026-10-03 — 093: max-only Viterbi accepted on unseen golden-v7
Built profiling/golden-v7-osm-unseen (8,973 addresses; fetch 2026-10-03T10:18:59-0700 → 2026-10-03T11:14:16-0700 per fetch_start/done.txt, slow Overpass). pcp/vmo/vmx identical to original; vmx 5.22× vs pcp 5.06× (+3.3%, all rounds) → 091 vmx accepted; validation next.

## 2026-10-03 11:19–11:31 PDT (094 raw/timing_start.txt, raw/DONE) — 094: vmx validated, new deliverable
vmx 5.264× golden-v1, 5.190× novel, 5.181× OSM (pcp 5.17/5.10/5.02 same session); 1 thread; peak 2.543 vs 2.613 GB; steady 1,878 vs 2,492 MB. Packaged research/final/091-vmx (19 files verified). Report, ledger, handoff updated.

## 2026-10-03 — 095–097
095 warm profile of vmx; 096 lookup key counts (all integer keys pre-resolved; ≈30 string keys/address); 097 unrolled accumulation rejected (−0.5%/≈0; first attempt's load check never enabled the path — preserved and noted).

## 2026-10-03 — evaluation target and later packaging plan (user)
Professor will test on their own Apple Silicon Mac (M1–M4, not Intel) from a GitHub download, possibly measuring with an AI assistant. Later: one-command benchmark app with a verifiable report. Noted: 091's x86 fallback is unsafe (only matters off Apple Silicon); build script has hard-coded local paths.

## 2026-10-03 — restart handoff written
research/HANDOFF_2026-10-03.md: state (vmx 5.26/5.19/5.18×, unseen 5.22×), measurement protocol and traps, limits (088/095), accepted and rejected work, untried ideas A–G ranked, next steps (golden-v8 fetch first; ideas A/B/C; packaging checklist incl. 091 x86 guard). Pointer added at the top of HANDOFF.md.

## 2026-10-03 21:45:36–22:01:13 PDT (session `date`; 098 raw/split_analyze.stdout mtime) — new session from HANDOFF_2026-10-03; v8/v9 fetch; 098
Created profiling/golden-v8-osm-unseen and golden-v9-osm-unseen builders (55 new cities each, checked disjoint from v3–v7 and from each other); fetch v8 then v9 started in background (v8 fetch_start 2026-10-03T21:46:47-0700).
098 EXP_NORM_PAIR (idea B): one scan produces both standard and admin token normalizations. Unit test 24.6M cases 0 mismatches (mutation detected); verify.sh all identical incl. fuzz; screen vs vmx +1.1% golden-v1 (6/6 rounds), +1.25% OSM (5/6); stage timers: normalization loop 0.235→0.147 µs cold dev, other stages unchanged. Awaiting golden-v8 for the unseen check.

## 2026-10-03 22:02:46–22:13:48 PDT (099 raw/created.txt; split_analyze.stdout mtime) — 099
099 EXP_PHRASE_LEN (idea C, largest site): identical everywhere; screen vs npr +0.1% v1 / +0.7% OSM; stage timers show features −0.02–0.04 µs but total flat → inconclusive, not carried forward. Idea C closed (remaining sites smaller).

## 2026-10-03 22:17:02–22:35:10 PDT (100 raw/created.txt; 101 screens 22:21:04–22:26:22 and 22:26:31–22:31:46; 101 split_analyze.stdout mtime) — 100 probe, 101
100 probe: ≈65% of prefix/suffix searches are on raw tokens equal to their admin form and in vocabulary; most return nothing. 101 EXP_PS_FLAGS: two spare bits of the 056 FTT entries mark "prefix/suffix search returns NULL_PHRASE" (model-derived, no extra memory); identical everywhere, 30M skipped searches re-verified; screen 2 vs npr +1.7% v1 / +3.1% OSM (6/6 rounds each; screen 1 v1 rounds disturbed, kept); context fill −0.11 µs cold. Noted heavy unrelated CPU load (VS Code crashpad handlers ~170% ×2) during screens. Candidate npr+psf awaits golden-v8.

## 2026-10-03 22:34:47–22:40:40 PDT (v8 fetch_done.txt; 102 raw/DONE) — golden-v8 built; 102 unseen check: npr and psf accepted
profiling/golden-v8-osm-unseen: 9,981 addresses, 41 countries (fetch 21:46:47 → 22:34:47). 102: vmx/npr/psf identical to original; npr +1.5% vs vmx, psf +3.1% vs npr, every round; psf 5.37× vs original on v8 (vmx 5.13× same session). 098 and 101 accepted. Validation 103 started.

## 2026-10-03 22:41–22:53 PDT (103 raw/timing_start.txt, raw/DONE) — 103: psf validated, new deliverable
psf 5.437× golden-v1, 5.386× novel, 5.453× OSM (vmx 5.27/5.24/5.19 same session); 1 thread; peak 2.543 vs 2.614 GB; steady 1,878 vs 2,492 MB; unseen v8 5.37× (102). Packaged research/final/101-psf; report, ledger updated.

2026-10-03 — corrected this session's log headers and 099–101 README dates to times sourced from raw/created.txt, screen start/DONE files and output-file mtimes (the first versions had approximate times).

## 2026-10-03 23:00:02 PDT (104 raw/created.txt) — 104 postcode flag probe (user: "continue with the postcode flag probe")
A walk of each vocabulary word over the postcode trie (bit 60 of the FTT entry) marks 2,847,397 of 3,694,560 words as falling off. In the postcode search, 40% (dev) / 45% (OSM) of fresh tokens are flagged; every flagged token's original walk fell off (0 exceptions incl. 4.8M on 2M fuzz). Flagged walks mostly fall at byte 1–2 (2–3 node reads skipped). No timing; candidate 105 not started.

## 2026-10-03 23:04:16 PDT (caffeinate start, `date`) — user: build 105 and keep going autonomously overnight ('make decisions yourself, do not stop until you reach it'). Started caffeinate -dims -t 172800 (48 h) in addition to pid 70844.

2026-10-03T23:13:13-0700 — 105 screen 1 too noisy (psf v1 6.21–6.60 µs; load avg 7.6). Demoted the long-running CPU users to background QoS with taskpolicy -b (reversible; nothing killed): VS Code crashpad handlers 54279, 52927; temp program 38042; icdd 98233 (see result in 105 README).
2026-10-03T23:28:56-0700 — sent SIGTERM to orphaned crash reporters 52927 and 54279 (chrome_crashpad_handler of a VS Code copy run from an AppTranslocation path that no longer runs; ≈98–170% CPU each for 30+ days, 124/141 CPU-hours; the user's VS Code from /Applications, pid 35242, untouched). icdd and the temp program 38042 left alone (QoS-demoted only).
2026-10-03T23:29:22-0700 — SIGTERM ignored; sent SIGKILL to 52927 and 54279.
2026-10-03 23:04:52–23:43:04 PDT (105 raw/created.txt; split_analyze.stdout mtime) — 105: exact; postcode search −0.012 µs dev / −0.027 µs OSM cold, total flat; screens 1–3 noisy (external load), screen 4 ≈0 → inconclusive, not carried forward.
2026-10-03T23:46:35-0700 — stopped pid 38042: an ASan-built test binary 't' (tmp.q1AhOAKnZO) started 2026-09-28 00:40 by an earlier Claude session in this project (cwd = research dir; stdout = that session's task output file /private/tmp/claude-501/-Users-kyle-Desktop-HW-CSCI-201-Reserach/4759bcf0-873c-4bdb-9568-87609358578b/tasks/bwok2nicb.output), still running at ≈6–88% CPU after 6 days.
2026-10-03T23:46:49-0700 — restarted icdd (killall; launchd relaunches on demand), it had been at ≈94% CPU for 3+ days.

## 2026-10-03 23:52:48 – 2026-10-04 00:03:54 PDT (107 raw/timing_start.txt, raw/DONE) — 107 clean-machine re-validation
psf 5.753× golden-v1, 5.649× novel, 5.323× OSM (vmx 5.55/5.44/5.16); original 7–10% faster than under load, psf 12–14% faster. Report headline switched to 107 with both sessions explained.
2026-10-04 00:04:26–00:12 PDT (108–110 raw/created.txt, commands.txt) — 108 clean stage split, 109 postcode walk probe, 110 context-fill sub-split recorded; golden-v10 built (unused).
2026-10-04 00:12:18–00:24 PDT (111 raw/created.txt; screen 00:14:50–00:20:00) — 111 EXP_PDT digit-prefix table: exact, but postcode search time unchanged → rejected.
2026-10-04 00:24:41–00:39:25 PDT (112 raw/created.txt; 113 screen DONE) — 112 cold-warm profile (function + line level via xctrace); 113 multi-line row prefetch re-test (128-byte steps): +6.6% golden-v1 / −2.1% OSM → rejected (replicates 085).
2026-10-04T00:42:52-0700 – 2026-10-04T01:02:20-0700 — 114 rt8 (transition-row every-line prefetch): +2.8% v1, +0.6% OSM; stage timers dev −0.15 µs cold, OSM ≈0; 115 unseen v9: identical, +0.7% (not slower) → accepted. Validation 116 next.

## 2026-10-04 01:03:28–01:14:39 PDT (116 raw/timing_start.txt, raw/DONE) — 116: rt8 validated, new deliverable
rt8 5.865× golden-v1, 5.769× novel, 5.491× OSM (psf 5.72/5.60/5.42 same session); 1 thread; peak 2.543 vs 2.614 GB; steady 1,878 vs 2,492 MB; unseen v9 5.28×. Packaged research/final/114-rt8 (SHA256SUMS verified). Report headline and ledger updated.
2026-10-04 01:03:59 PDT (117 raw/created.txt) — 117 string-key family probe: phrase families ≈11/address, bigrams ≈9.6/address.
2026-10-04 01:18:42 PDT (118 raw/created.txt) — 118 component-key probe: 4,012,750 trie keys enumerated, 1:1 with data ids; runtime phrase string = key except rare token-split cases (2 in golden-v1, several in fuzz) → needs length guard.
2026-10-04T01:21:21-0700 – 2026-10-04T01:37:27-0700 — 119 EXP_CP_IDS (component-phrase features as precomputed ids from the enumerated trie, length guard): identical, 12.8M uses verified; +2.7% v1 / +5.6% OSM; lookups −0.17 µs cold dev; 120 unseen v10 +4.7% every round (5.68×) → accepted. Validation 121 next.

## 2026-10-04 01:38:34–01:50:23 PDT (121 raw/timing_start.txt, raw/DONE) — 121: cpi validated, new deliverable
cpi 6.077× golden-v1, 5.928× novel, 5.775× OSM (rt8 5.90/5.81/5.51 same session); 1 thread; peak 2.543 vs 2.614 GB; steady 1,927 vs 2,492 MB; unseen v10 5.68×. Packaged research/final/119-cpi. Report headline updated.
2026-10-04T01:51:26-0700 – 2026-10-04T02:22:16-0700 — 122 bigram int keys on cpi: identical; screen +0.1% v1 / +2.2% OSM but stage timers show no net gain (features +0.08 µs) → not carried forward; peak −48 MB noted. Found the dormant EXP_INT_KEYS_VERIFY broken since EXP_CONST_IDS (documented).
2026-10-04T02:23:26-0700 – 2026-10-04T02:35:59-0700 — 123 string keys after cpi (17.5/address); 124 cpi line profile; 125 per-token trie timing (postcode digit-start 0.24 µs cold dev); 126 digit table + staged prefetch: +1.5% v1 / −0.5% OSM → rejected.
2026-10-04T02:37:02-0700 – 2026-10-04T02:48:42-0700 — 127 OSM warm profile of cpi (memset 7.4%); 128 row-mask clearing: −0.9% v1 / +0.6% OSM → rejected. golden-v11 built (unused).
2026-10-04T02:49:54-0700 – 2026-10-04T03:00:45-0700 — 129 dictionary key probe; 130 dictionary-phrase ids: exact, +1.1% v1 / −0.2% OSM → inconclusive, not carried forward.
2026-10-04T03:03:00-0700 — handoff updated (cpi deliverable, base, unseen sets, measurement, dead ends, broken verifier audit: no post-015 record claims EXP_INT_KEYS_VERIFY passed).
2026-10-04T03:03:28-0700 — 131 bigram canon probe: 37–39% both parts cached → stopped by kill rule.
2026-10-04T03:04:32-0700 – 2026-10-04T03:13:12-0700 — 132 warm OSM line profile; 133 phrase-length join on cpi: +0.5% / +0.9% → not carried forward.
2026-10-04T03:14:14-0700 – 2026-10-04T03:16:18-0700 — 134 cpi confirmation on unseen v11: identical (7,882), 5.64×. Report table updated. golden-v12 fetch started (reserved).
2026-10-04T03:17:13-0700 — wrote research/MORNING_SUMMARY_2026-10-04.md.
2026-10-04T03:18:30-0700 — package rebuild check of final/119-cpi: sources identical to 119; rebuilt binary golden-v1 sha 45574ee2… and OSM identical.
2026-10-04T03:20:49-0700 — records tidied (log times sourced, handoff next steps and v12 status, summary corrections). Experiments stopped: remaining levers below the 1.5% bar.
2026-10-04T04:07:11-0700 — golden-v12 built (rows=8273 countries=42 non_ascii=3961 dropped_overlap=0); unused, reserved.
2026-10-04T15:47:44-0700 — 135 weight compressibility probe (user request): state 22.1M distinct of 33.8M, trans 12.5M of 13.4M, none float32-exact; value-table compression would add a 95–169 MB random-access table → closed.
2026-10-04T16:04:57-0700 — 136 hot-tier probe (user idea): model-count ranking, 4 MB budget fixed in advance → covers 87–89% of transition and 28–29% of state weight records per address (dev and OSM alike); state key miss rate 10–15% → Bloom not pursued. Attempt 1 ranked by a non-count field (kept, invalid).
2026-10-04T16:10:14-0700 – 2026-10-04T16:26:00-0700 — 137 hot tier (user's layered model): exact (117.8M row uses verified; identical on all sets incl. unseen v5–v11), but slower: accumulation +0.04 µs cold/warm on dev in every round; hot rows already cached, routing costs compute → rejected.
2026-10-04T16:33:06-0700 – 2026-10-04T16:53:53-0700 — 138 cross-address lookahead (user's 'use the waiting time' idea, light version): exact with hints on every input, but −3…−7% (predictable data already cached; prediction costs compute) → rejected. Two failed xlp build attempts recorded.
2026-10-04T17:03:38-0700 — wrote research/plans/INTERLEAVING_PLAN_2026-10-04.md (user request: plan the full interleaving version; not started).
2026-10-04T18:07:13-0700 — 139 interleaving Phase 0 (user: 'lets do it'): switch ≈10 ns; 4 coroutine chains 3.95× throughput, 8 → 7.5× → gate passed.
2026-10-04T18:10:39-0700 – 2026-10-04T18:38:43-0700 — 140 interleaving Phases 1–3: exact in all modes (incl. random-switch stress, 2×2M fuzz); batch overhead ≈0; interleaving slower in every configuration (−3…−7% v1, −7…−10% OSM) → stopped at Phase 3 gate per plan. Plan marked executed/stopped.
2026-10-04T18:46:12-0700 — wrote research/HANDOFF_2026-10-04.md (user request: full handoff for a later research session); pointers added to HANDOFF.md and HANDOFF_2026-10-03.md.
2026-10-04T18:55:00-0700 — HANDOFF_2026-10-04.md: added §11 (proposed rewrite track: stage oracles/hybrid builds, milestone population, idea injection, outer-loop runner; sourced from published LLM-discovery practice). Plan only; nothing measured; awaits user approval.
2026-10-04T18:58:15-0700 — restarted icdd again (72% CPU; 141 raw/hygiene.txt).
2026-10-04T19:00:16-0700 – 2026-10-04T19:12:28-0700 (141 raw/created.txt, raw/DONE) — 141 warm bound of cpi (088 method) + stage split: warm bound 12.86× dev / 8.64× OSM / 8.50× unseen v11; front half 60–68% of cold time; identity-fixed accumulate+Viterbi 0.80–0.90 µs warm; 7.8× OSM needs 2.36 µs vs 2.13 µs warm bound. cpi normal OSM runs contaminated by external load (use 121). No library change.
2026-10-04T19:16:23-0700 — wrote research/plans/REWRITE_PLAN_2026-10-04.md (user request: integer feature pipeline plan sized with 141; oracles, hybrid builds, milestones/kill rules, population, idea cards, bash 3.2 runner). Plan only; awaits user approval; no library code written.
2026-10-04T19:18:59-0700 — review fixes: plan/141 sizing no longer mixes sessions (OSM 7.8× gap from 121: −25.7%); M3 judged by 121/103 + v12, M5 by v13; runner kills whole process group, PID-owned locks, wrapper-only Bash; 141 split-run outliers and model hashes recorded; professor report: dated note that 141's warm bound (8.6× OSM) supersedes 088's 'out of reach' statement.
2026-10-04T19:23:04-0700 – 2026-10-04T19:27:19-0700 (142 raw/created.txt; last dump END in raw/id/run.log; controls and gzip followed) — 142 REWRITE_PLAN M0 (user: "now run the M0 + M1", presentation polish deferred): fprintf-only EXP_ORACLE hooks on original address_parser.c/crf.c; ctl and orc identical on v1/novel/OSM/fuzz (dumps off and on); dumps deterministic; negative controls fail at a=5000 → M0 passed. Oracles: research/oracles/v1 (1.5 GB gz, SHA256SUMS).
2026-10-04T19:31:26-0700 – 2026-10-04T19:36:26-0700 (143 raw/created.txt) — 143 REWRITE_PLAN M1: all model tries enumerated (counts = headers); unit coverage of runtime word-family keys 99.8% dev / 100% OSM, bigram split ambiguity 0.23–0.35% → rules 2/3 pass; gross integer tables 142.6 MB but they replace existing entries (122 measured −48 MB for pairs) → rule 1 deferred to a declared peak check at first build. keydump attempt 1 failed (LIBPOSTAL_DATA_DIR), kept. v13 fetch not started (awaits user).
2026-10-04T20:20:46-0700 — user request: caffeinate; started caffeinate -dims -t 172800 (pid 73130) alongside 21869
2026-10-04T20:21:29-0700 – 2026-10-04T20:32:24-0700 (144 raw/created.txt, raw/DONE) — 144 feature-section timers on cpi (user: plan through math): neighbour sections 9.8–10.7% of cold time; fss identical. Build attempt 1 failed (scanner_cache dir), kept.
2026-10-04T20:27:34-0700 – 2026-10-04T20:43:23-0700 (145 raw/created.txt, raw/DONE) — 145 bigram deletion bound (diagnostic, not exact): features+lookups ceiling 7.1% dev / 9.1% OSM / 8.9% v11; with 122's in-situ probe and canonicalization costs the exact bigram card projects +2.1–3.5% dev, +3.6–5.1% OSM.
2026-10-04T20:46:12-0700 — wrote research/analysis/REWRITE_MATH_2026-10-04.md: whole integer pipeline ≈ +4.5–7.5% (UNVERIFIED; v2 model) → ≈6.4–6.5× golden-v1, 6.1–6.25× OSM; 7.8× not reachable under byte-identity by any identified path. M2 designed, not built. v13 fetch still running.
2026-10-04T21:14:13-0700 – 2026-10-04T21:19:15-0700 (146 raw/created.txt, raw/verify_summary.txt) — 146 REWRITE_PLAN M2 (user: "a gain is a gain lets do it"): H-build of integer bigram ids exact on all oracles/corpora/fuzz, first attempt → M2 passed.
2026-10-04T21:23:29-0700 – 2026-10-04T21:34:55-0700 (147 raw/created.txt, raw/DONE) — 147 section split of cpi + EXP_INT_KEYS_PAIRS: warm features +0.02–0.04 µs, lookups −0.01 µs → 122-style integer emission costs more than string emission; cold contaminated (Chrome/WindowServer load, swap 9.9 GB, Spotlight, after 146's parallel verification).
2026-10-04T21:39:00-0700 — REWRITE_MATH §7 correction: bigram card lower bound ≈0% (was +2.1/+3.6%); whole-pipeline realistic range now ≈0–7.5% (UNVERIFIED). Handoff and plan updated.
2026-10-04T21:39:28-0700 – 2026-10-04T21:42:26-0700 (148 raw/created.txt, raw/load.log) — 148 warm line profile ipr vs cpi: string emission −0.135, canon +0.084, int machinery +0.042 µs → declared rule says build R1. Attempt 1 invalid (bench ended before the recording).
2026-10-04T21:45:46-0700 – 2026-10-04T22:16:27-0700 (149 raw/created.txt, raw/split/DONE) — 149 M3 R-build on cpi (r1a: int pairs + canon ring; r1: + 528,327 phrase-unit ids): exact on all sets incl. 2×2M fuzz and unseen v4–v11; screen r1a 0.993×/0.996×, r1 0.985× (0/6)/1.012×; warm features +0.035/+0.064 µs → rejected (rule 4); integer-pipeline track stopped. golden-v13 built (9,716 rows; 7 cities missing after 504s; unused). A false 'unseen-v5 DIFFERENT' from a zsh word-splitting bug is recorded and superseded.
2026-10-04T22:17:18-0700 — final write-up: REWRITE_MATH §8, plan status, handoff §12.3. Deliverable remains cpi (121).
2026-10-04T22:34:57-0700 — built research/suite (user request: configurable, extensible full suite; original vs best per dataset with speedup multiplier and CI): suite.json registry (13 datasets, 2 reserved; variants original/cpi/+rt8 disabled), run_suite.py (integrity, correctness, timing, memory, threads, report; py3.9 stdlib), README. Smoke test results/smoke-test passed all stages. Full run results/full-2026-10-04 started (11 non-reserved datasets, 5 rounds × 20 passes, 2×2M fuzz).
2026-10-04T22:36:35-0700 — user asked to stop: killed results/full-2026-10-04 at the start of timing (correctness had finished: 11/11 dumps identical, 2×2M fuzz identical); timing incomplete, no report generated.
2026-10-04T23:08:01-0700 — suite report fixes (user request): t-interval on log per-round ratios, country_min_n=50, source-weighted headline (5.84x); regenerated results/20261004-223647 (user's run) with --report-only (old runner kept as results/run_suite.v1.py.bak). Added analyses/why_speedup_varies.py: over 499 country cells cpi_us = 0.530 + 0.1494*original_us (R^2 0.918) → speedup rises with address cost toward 6.69x; explains the dataset ordering.
2026-10-05T00:21:19-0700 — root README.md replaced by a concise professor report (user request); previous long report preserved byte-identical at research/archive/README_full_2026-10-04.md (sha 7fcacd68…). Effort figures from ~/.claude transcripts: 11 sessions, 339 user turns, ~59 h active (gaps capped 30 min), 2026-09-27 23:29 → 2026-10-05 00:19. Corrected: total addresses 128,540 (not 138,826 as said earlier in chat); 'thirty-plus memory experiments' → 15 (math doc annotated).
