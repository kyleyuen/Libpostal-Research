> **Current state: [HANDOFF_2026-10-10.md](HANDOFF_2026-10-10.md)** (203 streamed CPI candidate validated and promoted for measured default-parser scope; 119 and 202 remain historical records).

> **Historical handoff: [HANDOFF_2026-10-07.md](HANDOFF_2026-10-07.md)** (state through experiments 188–201; its no-promotion status predates experiments 202–203).

> **Start here: [HANDOFF_2026-10-06.md](HANDOFF_2026-10-06.md)** (state after 158–162: redesign closed by kill rules; 161fx = cpi + both API fixes at speed parity; next = promote/validate 161fx, user decision).

> **Superseded as the starting point by `HANDOFF_2026-10-04.md`** (consolidated state, measurements, lessons and remaining ideas through experiment 140).

> **Start here: [HANDOFF_2026-10-03.md](HANDOFF_2026-10-03.md)** (restart summary: state, lessons, what failed, untried ideas, next steps). This file keeps the running history.

# Handoff — libpostal parser speed research (stopped 2026-10-02 12:29 PDT)

Read first: `../README.md` (the report), `METHOD.md` (rules), `experiments/README.md` (ledger), `../AGENTS.md` (record-keeping rules).
The consolidated [audit](analysis/FINAL_AUDIT_2026-10-01.md) is the current source/claim verdict.

## Stop state

The user explicitly said **stop and create a handoff**. Do not resume builds, model scans, benchmarks, or experiments automatically, even if AC power returns. The last power status reported by the user was off AC. At this handoff, the Linux timing runner had already stopped at a safe boundary (`experiments/026-cross-platform/linux/timing-dense-3x150/STOPPED`), all subagent work was complete, and a targeted process check found no benchmark, dumper, or timing runner. No task is waiting to continue in the background.

The 021 Mac result is the only accepted speed result. Linux's 3.5225× is a partial, noisy diagnostic; Windows has no runtime measurement. Experiment 028 is source-only preparation. See the numbered records before considering any further work.

## Scope decision 2026-10-02 (user)

The user parked the Linux track ("we are done with it") and limited further work to **macOS only**. Open items 1 (Linux), 2 (Windows) and 3 (portable memory without mimalloc) are therefore closed unless the user reopens them; the 026 records stay as written. Next step proposed (not started): a stage-timing build of 021 `l4` to get current per-stage costs before choosing any new speed candidate.

## 2026-10-02 session results (029–031)

- 029: current stage split of `l4` (context fill 2.18, features 2.03, lookups 1.15, accumulation 1.09, Viterbi/rest 1.00 µs); original/`l4` 2.90× fewer instructions, 4.31× fewer cycles. Paper limits in `analysis/LIMITS_2026-10-02.md`.
- 030: 57% of lookups still use string keys; existing pair routing −1.4% instructions, no time gain → not adopted. Phrase-id keys not built (needs an exactness proof; expected gain a few percent at most).
- 031: PGO does not overfit (cross-trained on golden-v1/novel/OSM, all within noise; no PGO costs 2–4.5%). **Memory finding:** same-session steady footprint original 2,172 MB vs `l4` 2,250 MB (+78 MB), so `l4` fails the steady-memory rule in that state. `l3` (2,141 MB, ~0.5% slower) and `m3` (2,026 MB) pass. The `final/021-l4` package and its BUILD.md were left unchanged (hashes intact); the report carries the qualification. The user chose `l3`; experiment 032 validated it (4.38× / 4.33× / 4.28×, identical output, 1 thread, peak 2,380 MB and steady 2,141 MB vs original 2,614 / 2,172 MB) and it is packaged as `final/021-l3/`. **`l3` is the deliverable.**
- 033: context fill split — component-phrase trie search ≈1.0 µs, postal codes ≈0.45 µs. 034: parallel timing distorts results (+6–13% per process, biased pair ratios); timing stays sequential, builds/checks may be parallel. 035: measured ceiling (oracle, probe only) 2.63 µs = 13.0× golden-v1, 13.1× novel, 11.4× OSM if lookups were also free; **realistic ceiling (lookups paid) 3.91 µs = 8.7× / 8.6× / 7.6×**; 036–037: trie-walk counts and lockstep walks (rejected, 3–5% slower); the user's target is ~90% of the ceiling, which needs a phased front-half rewrite (integer pipeline).
- 036–040 (evening): trie walks are first-touch-miss-bound (038: warm repeat +952 cycles vs ≈5,800 first time); lockstep walks rejected (037, 3–5% slower); word-walk table needs 30–50 MB, not pursued (039); feature generation warm cost ≈17k instructions ≈1.0 µs of 2.15 µs (040). Best remaining candidate: leaner feature generator under a shadow-mode equivalence check (est. 6–12%, unverified). Timing must stay sequential (034).
- 041–051 (night): warm bound 3.67 µs (9.2×); 53% of time is first-touch memory; superpages unavailable; trace 315 lines/174 pages per address; early prefetch (046) and lazy transition rows (050) rejected; **048 `va` (Viterbi-20 + transition alias) validated in 051: 4.53× / 4.55× / 4.62×, packaged as `final/048-va` — the deliverable.** Reachability analysis `analysis/REACHABILITY_2026-10-02.md`: optimistic sum of identified levers short of 7.8×; options put to the user. A session-wide `caffeinate -dims -t 36000` was started at the user's request.
- 052–055 (late night, usage limit reached mid-work): memory map (postcode graph 487 MB largest); 053 exact postcode-graph compression −323 MB steady (speed −0.6%, kept as enabler, not packaged); 054 per-word vectors rejected (8% coverage, −0.4%); 055 weights 65–93% distinct (no value table). Deliverable remains `final/048-va` (4.53/4.55/4.62×). Next ideas if resumed: fused key+row records, component first-token table funded by 053's freed memory, quiet-machine re-measure.
- 056–057 (after the limit reset): exact first-token table for the component trie (0 mismatches in 7.29M verified walks) + role-row prefetch: +2.5% golden-v1, +1.5% novel, −0.7% OSM vs `va` in full validation → not adopted (needs 053's compression to fit memory; OSM regression likely from postcode decoding). Memo reuse (`ftm`) rejected. `va` remains the deliverable.
- 058–064 (2026-10-03): numeric postal table and checkpoint index rejected (058); name-byte keys +3.2–3.8% (059) and extended to all helpers +2.2–3.2% (061); all-letters normalization fast path +0.5–1% (062); address-dictionary first-token table rejected (063). **064 validated `lf` = 4.92× / 4.86× / 4.91×, packaged as `final/062-lf` — the deliverable.**
- 065–069: phrase-string cache (067, +0.5%) accepted into the stack head `pl`; 066 and 068 rejected. At the user's request, built `profiling/golden-v4-osm-unseen` (10,293 addresses, 56 new cities) and re-measured (069): `lf` 4.96×, `pl` 5.02×, outputs identical — gains generalize.
- Overnight 2026-10-03 (full autonomy granted): 070 compiler re-tune (no gain); 071 certified early exit measured infeasible (margins ≪ bounds); 072 digits (neutral); 073/076 re-profile; **074 FTT prefetch pass (+2.4%) and 077 postcode-row prefetch (+1.35%) accepted**; 075, 078, 079 (dataset-specific, caught by unseen v5), 081 rejected; golden-v5 unseen set built; 080 unseen re-measure; **082 validated `pcp` = new deliverable `final/077-pcp` (5.15×/5.07×/5.06×, unseen 5.04×)**.
- 083–089 (2026-10-03 early morning): 083 workload counters (pcp gain smaller on OSM-style data, not fewer targets); 084 early postal prefetch, 085 multi-line row prefetch (+4.7% golden-v1 but −1.6% unseen v6, 086), 087 dense rows — all rejected; golden-v6 built; **088 warm bound of pcp: 11.1× golden-v1, 8.1× OSM, 7.7× unseen v6 → 7.8× on OSM-style data unreachable by memory-side work**; 089 warm function profile: no exact change above 2–3% left. See analysis/REACHABILITY_2026-10-03.md. Next step needs a user decision: stop at pcp (5.06× unseen) or commission the front-half rewrite.
- 2026-10-03 daytime (user: 'continue'): 090 Viterbi split; **091 max-only Viterbi (vmx) accepted** after unseen golden-v7 (093: +3.3% over pcp, 5.22×); 092 touched-rows measurement (not viable); **094 validated vmx = new deliverable `final/091-vmx` (5.26×/5.19×/5.18×)**.
- Evaluation target (user, 2026-10-03): professor's own Apple Silicon Mac (M1–M4, never Intel), code downloaded from GitHub; professor or an AI assistant will measure. Later deliverable (not started): one-command app that builds original + ours, runs both benchmarks, checks identical output, writes a verifiable report. Before release: guard 091's backtrack to the aarch64 forward path (x86 fallback unsafe), remove hard-coded /Users/kyle and Homebrew paths, fold experiment switches into a clean tree.
- User rules this session: macOS only; no tuning to our datasets (tables from the model only; corpora only for checking/measuring).

## Where things stand

**Final deliverable: 021 `l4`** — packaged in `final/021-l4/` (sources, build script, hashes, `BUILD.md` with the exact build command).
Measured 2026-10-01 06:47-07:14 on AC, same-session original: golden-v1 4.33x, novel 4.36x, floor (non-ASCII) 4.27x, golden-v3-osm 4.31x,
ASCII 4.30x. Byte-identical (dumps, frozen references, 2 x 2 M fuzz). Single thread. Peak 2.471 GB vs 2.614 GB; steady 2,250 vs 2,492 MB.
Load 8.7 s vs 3.4 s. Build: Apple clang 17, whole-library PGO (golden-v1 development only) + LTO, mimalloc linked statically, tokenizer -O1.
A later re-time (022 session) gave l4 4.31-4.41x.

**Experiment 025:** the packaged `l4` aborts if `libpostal_parser_print_features(true)` is enabled (exit 134). A separate `fixes` build repairs packed-row fallback scoring, teardown state and save guards; output is byte-identical to the original on golden-v1, novel and OSM and it remains single-threaded. `borrow` (avoid allocating already-lowercase ASCII normalization) is rejected: paired long confirmation 0.999× on golden-v1 and 0.991× on novel versus `fixes`. Full records, patch and raw commands are in `experiments/025-audit-next/`. The unchanged 021 `l4` remains the measured speed deliverable. OSM was used during 020/021 selection, so its 021 result is not a strict held-out estimate.

Integrity check done at end of session: all 15 files in `final/021-l4/` and all 18 files of `experiments/021-final-stack/artifacts/l4/` match their
hashes, and `experiments/019-fixed-costs/candidate/` (the shared working sources) is byte-equal to `final/021-l4/sources/`.

## Portable / other-machine track (experiments 022–028)

- **022 `pt`** = l4 without NEON, PGO, LTO, mimalloc: byte-identical, **3.74-3.86x** (≈87% of l4's speed is code, not toolchain), but **memory fails**:
  peak 3.176 GB, steady 3,029 MB (> original). So l4's memory result depends on mimalloc.
- **023 diagnosis**: load builds and frees ~1.1 GB of temporaries (feature/vocabulary tries 470 MB; full feature dictionaries 630 MB, freed after the
  integer split in `crf_int_keys_enable`). The macOS system allocator keeps ~665 MB of freed large blocks dirty; mimalloc returns them. Grows with
  sparsity (dictionaries are sized by the load factor).
- **024 option A** (`malloc_zone_pressure_relief` after load): **no effect on macOS** — rejected and reverted from the shared sources.
- **026 Linux ARM64 Docker VM**: the pinned original and exact 021 source give byte-identical output on golden-v1, novel and OSM; one parsing thread was observed. The exact 021 settings **fail memory** on Linux: 2,347,384,832 bytes peak RSS with glibc versus 1,960,681,472 for the original; active mimalloc variants are higher still. A separate `candidate_dense` removes only `EXP_LOADF=4`, preserves output, and used about 14–15 MB less peak and late RSS than the original in paired golden-v1, novel and OSM runs. A noisy partial golden-v1 diagnostic stopped at the safe boundary after nine runs: dense/original paired warm median 3.5225×, with only 1/447 original warm passes clean, no novel run and no true cold pass. This is not an accepted Linux speed result. Linux peak RSS is not directly comparable to the macOS footprint metric.
- **026 Linux x86-64 under emulation**: original and exact 021 source build with upstream SSE2 flags and produce byte-identical ordered output on all three corpora. Emulation supplies functional evidence only, not native x86 speed or memory.
- **026 Windows x64**: original and Windows-adapted 021 cross-compile to PE DLLs and paired harnesses. The final `l4_r3` port fixes two sparse-arena commitment defects found in source review. No Windows machine or VM is available, so output, speed, thread count and memory are unverified; mimalloc is not active in that binary.
- **027 root-transition cache**: an exact parser-owned cache passed output, fuzz and one-thread checks but gave 0.9980× pooled development throughput versus unchanged 021 on five noisy paired runs. Rejected as a speed change; no new gain established.
- **028 name dispatch**: a model-derived feature-name dispatch has source-only preparation; its inspector was not built or run, no candidate parser change exists, and there is no result. See its numbered record.

## Open items, only if the user explicitly resumes

1. **Linux speed and delivery:** the prepared `linux/run_timing_dense_r1.sh` and `linux/run_true_cold.sh` were not executed. The benchmark source's warmup error check was fixed after the diagnostic, but its binaries were not rebuilt or retested. On an explicit resume, first review those scripts and the 026 runbook, rebuild the harness, then collect quiet AC, same-session original/dense timing on golden-v1 and novel plus a true no-warm-up cold check. Judge output, one-thread, peak and steady memory, setup time, and speed together. The exact 021 Linux build already fails the memory rule.
2. **Windows runtime:** run the 026 paired PE harnesses on a real Windows x64 host or VM when available. Cross-compilation alone does not clear any runtime gate. The Windows mimalloc override needs its redirect DLL or explicit `mi_` calls (unverified), and the separate 025 feature-print repair is absent from `l4_r3`.
3. **Portable memory option C:** build the split integer/string tables directly from the trie so the roughly 630 MB full dictionaries never exist. It needs a separate numbered experiment, exact output checks, and same-session performance and memory evidence. The current 021 Mac package stays the measured deliverable unless a successor clears all gates. Mapped dictionary arrays (option B) are another idea but do not address the roughly 470 MB upstream tries.
4. **Other ideas offered but not done:** a fourth never-used held-out dataset (the OSM set was used in 020/021 decisions, so it is no longer strictly held out); instruction-count comparison (original vs l4) as machine-independent evidence that the gain is removed work.
5. **Further speed:** the actual 021 profile is in 026 and led to the rejected 027 root cache; 028 prepared a model-name inspector but has not tested the dispatch idea. The older function audit's C1 weight-row co-location and F5 hash-while-building percentages are unverified estimates from an earlier build. Do not claim them as new speedups. The 025 `borrow` attempt stays rejected.
6. **Maintenance repair:** before treating 025 `fixes` as a new release build, consider a targeted sanitizer/induced-allocation-failure check for source-level teardown risks. The optional feature-print crash repair itself was dynamically reproduced and output-checked.

## Practical notes for the next session

- Timing only on AC; `tools/env_gate.sh` pauses on battery or if any `research/PAUSE_TIMING*` file exists. Use `caffeinate` for long runs.
- Take timestamps from marker files (`start.txt`, `DONE`), never from memory — three premature timestamps had to be corrected this session.
- Wait on marker files or PIDs, not `pgrep -f`/`pkill -f` patterns (they matched their own command lines and once killed a build chain).
- Experiment scripts: `experiments/021-final-stack/tools/build_cc.sh` (build), `verify.sh`, `run_timing*.sh`; 022 has copies pointed at its folder.
- A GateGuard hook may block the first Bash command of a session until the request and the command's purpose are stated.
- The professor measures only parse time, with his own "fair, average" dataset; compile time ~5 min is acceptable.
