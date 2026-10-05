# Experiment 018 — Compiler-only builds of the 016 normalization changes (no source change)

Date/time/timezone: started 2026-09-30 22:40 PDT
Status: **complete.** Best: `c2s1` — **3.39x golden-v1, 3.39x novel, 3.30x floor**, byte-identical, one thread, peak 2.539 GB; compiler-only change on top of 016 (+3-5%).

## Hypothesis and change

Startup and compile time are not measured by the evaluation (compile time should stay "reasonable", a few minutes). 016 showed PGO + thin LTO of the 11
changed files adds ~10%, and that `t45` (T4 + T5) alone is at least as fast as `all`. libpostal itself builds the generated tokenizer `scanner.c`
(310,256 lines) at `-O0` ("can take a very long time to compile with higher optimization levels"); tokenizing costs ~0.5 µs per address (016 T2).
Variants, all with 016 `t45`'s source switches (k1 + EXP_TRANS_SKIP + EXP_NORM_TABLE), no source change:
- `c1`: PGO + thin LTO of the 11 changed files (as 016 `pgolto`, but on `t45`).
- `c2`: the whole library (41 sources + scanner at -O0 as upstream) recompiled with PGO + full LTO.
- `c2s`: `c2` with the scanner at -O2 (compile time recorded; -O1 if -O2 is unreasonable).
- `c3`: `c2s` with -O3 for the library sources.
- `c4`: `c3` with `-mcpu=apple-m1` (runs on every Apple Silicon Mac; no M4-only instructions).
PGO profiles are trained on `profiling/golden-v1/development.txt` only. Exactness: the same end-to-end checks as 016 (dumps, frozen references,
negative control already established, 2 x 2 M fuzz). Prediction: none.

## Log of work

- `c2` (whole library, PGO + full LTO, scanner -O0 as upstream): built in under a minute; byte-identical (dumps, frozen references, 2 x 2 M fuzz).
- Scanner compile time measured: **-O1 256 s, -O2 318 s** (once). A first `c2s` attempt compiled the scanner with LTO, which re-optimized it at every link
  (3 tools x 2 PGO builds) and was stopped after ~6 min; a stop command then missed one process, so two `c2s` builds ran concurrently — both stopped, their
  artifacts discarded. Fix: the scanner is compiled once per level without profile/LTO flags and cached (`artifacts/scanner_cache/`).
- `c2s1` (scanner -O1), `c3` (-O3, scanner -O1), `c4` (`c3` + `-mcpu=apple-m1`): built in ~30 s each from the cache; `c2s2` (scanner -O2) building.
- A verification queue again matched its own command line (same mistake as in 016) and was replaced by `tools/chain.sh`, which waits on a log line instead.
- `tools/run_timing.sh` (detached) waits for all builds and checks, then times original, 016 `k0`, 016 `pgolto` and every verified 018 build.

## Results (AC, 2026-09-30 22:53-23:41 PDT; no waits, no battery runs, no outlier passes)

Session A (`research/tools/run_unattended.sh`, 5 processes x 18 warm passes, rotating order; `raw/timing/A-summary.md`), speedup vs the original by
median (all estimators agree):

| build | flags | golden-v1 µs | vs original | novel µs | vs original |
|---|---|---:|---:|---:|---:|
| original | — | 34.42 | 1.000x | 34.55 | 1.000x |
| 016 `k0` (= 015 `k1`) | — | | 2.711x | | 2.731x |
| 016 `pgolto` | 11 files PGO + thin LTO | | 3.299x | | 3.243x |
| `c2` | whole library PGO + full LTO, scanner -O0 | | 3.341x | | 3.318x |
| **`c2s1`** | `c2` + scanner -O1 | **10.15** | **3.389x** | **10.20** | **3.388x** |
| `c2s2` | `c2` + scanner -O2 | | 3.400x | | 3.385x |
| `c3` | -O3, scanner -O1 | | 3.394x | | 3.393x |
| `c4` | `c3` + `-mcpu=apple-m1` | 10.11 | 3.405x | 10.16 | 3.399x |

Floor subset (5,404 non-ASCII addresses, 5 processes each; `raw/timing/B-floor-summary.md`): original 32.22 µs; `k0` 2.303x; `pgolto` 3.151x;
`c2` 3.216x; `c3` 3.277x; `c4` 3.281x; `c2s2` 3.294x; **`c2s1` 9.78 µs, 3.295x**.
Peak footprint (median of 10): original 2,613,332,176; `c2s1` 2,538,850,488; `c4` 2,538,850,488.
Compile time (one full build): library ~5 s + scanner -O1 256 s (once) + link ~3 s; with PGO the library and link are built twice (+~10 s); the cached
scanner is reused. Scanner -O2 costs 269-318 s and gives no further gain.

## Interpretation

- Recompiling the whole library together (PGO + full LTO) adds ~1.5-2.5% over 016's partial build; optimizing the generated tokenizer (upstream -O0)
  adds ~1.5-2% more. -O3 and CPU tuning add < 0.5% (within noise).
- Remaining distance to 3.5x: golden-v1 needs 9.83 µs (now 10.15, -0.32 µs, 3.2%); floor needs 9.21 µs (now 9.78, -0.57 µs, 5.8%).

## Decision

**Accepted: `c2s1` is the new best candidate** (3.39x / 3.39x / floor 3.30x): t45's source switches, whole-library PGO (development split) + full LTO,
scanner -O1. Chosen over `c4`/`c2s2` because they are within noise and `c2s1` needs no CPU-specific flags and the shorter scanner compile.
Next: experiment 017 (spend load time on precomputed tables), aimed at context fill (3.2 µs), feature building (2.7 µs) and lookups (2.2 µs).
