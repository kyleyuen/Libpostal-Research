# Experiment 029 — current per-stage split and instruction counts of 021 `l4` (measurement only)

Date/time/timezone: record created 2026-10-02T13:52:29-0700 (`raw/created.txt`), America/Los_Angeles.
Status: complete (measurement only; run 2026-10-02T13:52:56-0700 to 13:56:37-0700 per `raw/start.txt`, `raw/DONE`)

## Hypothesis and change

Measured bottleneck and why this change should help: No speed change. The only per-stage breakdown (016, ~11.8 µs/address) predates 017-021; the 026 profile of `l4` hides 27.8% of samples inside LTO-inlined code and was taken under heavy host load. Two candidates chosen from it (025 `borrow`, 027 root cache) gave ~1.00×. This experiment measures (a) current µs per stage of `l4` and (b) instructions and cycles retired per address for the preserved original and unchanged `l4`, so future candidates get an Amdahl ceiling and a low-noise screening metric.
Exact implementation change: none to parser logic. Stats build `l4s` = exact 021 `l4` sources and switches + `-DEXP_STAGE_STATS` (timers present in the sources since 016), built with the same procedure (Apple clang 17, whole-library PGO trained on golden-v1 development only, full LTO, mimalloc, scanner -O1). The timers add overhead and change the code PGO/LTO sees, so stage µs are diagnostic shares, not `l4` timings.
Base commit and candidate commit: pinned libpostal 25099c5; sources `experiments/019-fixed-costs/candidate/` verified byte-equal (sha256 of all .c/.h) to `final/021-l4/sources/` before the build.
Incremental comparison base: unchanged 021 `l4` (`experiments/021-final-stack/artifacts/l4/bench_ext`); original = `experiments/015-integer-keys/artifacts/bench_ext_baseline` (the harness 021 timing used).

## Reproduction

Hardware/OS/compiler: Apple M4 Pro, macOS 26.5.2, Apple clang 17.
Corpus: golden-v1 development split only, `experiments/025-audit-next/data/development.tsv` (10,308 lines, sha256 6e97d10f…68b9).
Model hashes: pinned set in `profiling/manifest.json`; not rehashed in this run.
Build/run: `caffeinate -i /bin/bash tools/run.sh` (build via `tools/build_cc.sh`, a copy of 021's with E=029 and `stage_bench` (016) added to the linked harnesses; differences: the E path, the harness list and the hash list). Scanner object copied from 021's cache (`raw/scanner_cache_copy.sha256`). Each step's command, timestamps, stdout/stderr and exit code: `raw/run.log`, `raw/<step>.{stdout,stderr,exit}`. Hashes: `raw/input_binary_hashes.txt`, `artifacts/l4s/hashes.txt`.
Method: stage timing = `stage_bench dev.tsv 50` (one untimed warm-up pass, 50 timed passes), 3 runs. Instructions/address = (I(22 passes) − I(2 passes)) / (20 × 10,308) from `/usr/bin/time -l` "instructions retired"; same for cycles. 3 rotating reps per binary. Env gate (AC/thermal) before each run; `raw/env_before.txt`, `raw/env_after.txt`.

## Results

All 16 steps exited 0 (`raw/*.exit`); AC gate passed before each of the 15 timed runs (`raw/gate.log`). Build `l4s` and `l4s_gen` took 9 s and 8 s (scanner object reused; `raw/compile_times.txt`); PGO profile `~/.cache/libpostal-020-pgo/l4s/dev.profdata` was trained on development only.

**Stage split of `l4s` (µs/address, development split, 50 timed passes, 3 runs; `raw/stage_r{1,2,3}.stdout`).** Timers included; shares are diagnostic.

| Stage | r1 | r2 | r3 | median | share of median total | 016 `all7` (old) |
|---|---:|---:|---:|---:|---:|---:|
| context fill (phrase search etc.) | 2.228 | 2.149 | 2.179 | 2.179 | 27.2% | 3.25 |
| feature generation | 2.056 | 1.998 | 2.027 | 2.027 | 25.3% | 2.67 |
| feature lookups | 1.146 | 1.125 | 1.204 | 1.146 | 14.3% | 2.18 |
| weight accumulation | 1.108 | 1.077 | 1.093 | 1.093 | 13.6% | 1.17 |
| Viterbi + rest of predict | 1.006 | 1.000 | 1.004 | 1.004 | 12.5% | 1.00 |
| tokenize | 0.225 | 0.220 | 0.222 | 0.222 | 2.8% | 0.54 |
| response build | 0.189 | 0.187 | 0.189 | 0.189 | 2.4% | 0.80 |
| normalize | 0.066 | 0.064 | 0.068 | 0.066 | 0.8% | — |
| response destroy + unattributed | 0.023 | 0.023 | 0.023 | 0.023 | 0.3% | — |
| **total** | 8.046 | 7.843 | 8.010 | 8.010 | | ~11.8 |

Per address: 39.03 bytes, 5.878 tokens, 45.86 state features, 8.74 transition features, 0.0003 bypassed. Peak footprint of the stats harness 2,477,033,776 bytes.

**Instructions and cycles per address** (difference method, `tools/analyze_ic.py` → `raw/analyze_ic.stdout`; raw `raw/ic_*`):

| Binary | instructions/address (r1, r2, r3) | median | cycles/address (r1, r2, r3) | median | IPC |
|---|---|---:|---|---:|---:|
| original (`bench_ext_baseline`) | 209,918 / 210,803 / 212,133 | 210,803 | 138,171 / 143,983 / 138,854 | 138,854 | 1.52 |
| 021 `l4` (`bench_ext`) | 72,591 / 76,567 / 71,050 | 72,591 | 32,219 / 36,350 / 29,087 | 32,219 | 2.25 |

Original/`l4`: **2.90× fewer instructions, 4.31× fewer cycles**; i.e. the 4.3× is about 2.9× from removed work and about 1.48× from higher IPC (fewer stalls per instruction). Warm pass medians in the 22-pass runs: `l4` 7.84–8.12 µs, original 34.46–36.51 µs (`raw/ic_*_p22_*.stdout`); these are not a controlled speed result.

## Interpretation and limitations

- Accumulation (1.09 µs) and Viterbi/rest (1.00 µs) are essentially unchanged since 016, while every other stage shrank; together they are now 26% of the time. Context fill (27%) and feature generation (25%) remain the largest stages. Accumulation is ~54.6 weight rows per address, ≈20 ns per row, consistent with memory-bound row reads (UNVERIFIED as a cause; no miss counters taken).
- The difference-method instruction count spreads by up to ±3.6% between reps (whole-process counts include ~9 s of model load that is not perfectly repeatable), so it can screen changes larger than about 5%, not 1–2% changes. Per-pass in-process counters would be tighter (not built).
- `env_snapshot.sh` was called without its FILE argument, so `raw/env_before.txt`/`env_after.txt` contain only its error; a snapshot taken right after DONE (`raw/env_posthoc.txt`, 13:57:10) shows AC, no thermal warning, load average 7.1/8.1/8.1 and two unrelated processes at ~171% CPU. The host was busy; timings are diagnostic.
- Stats-build timers change code layout/PGO; stage µs sum to 8.0 µs, close to unprofiled `l4` (~8.0 µs warm in the same window), so the distortion appears small but is not quantified.

## Decision

Measurement complete. Next candidates should be ranked by these stage ceilings (context fill 2.18, features 2.03, lookups 1.15, accumulation 1.09, Viterbi/rest 1.00 µs) and screened by instructions/address before paired timing.
