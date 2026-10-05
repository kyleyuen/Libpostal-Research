# Experiment 020 — Other compilers and allocators, plus session-2 timing of 019's follow-up builds

Date/time/timezone: 2026-10-01 ~02:05-05:02 PDT (unattended; end time from raw/timing2/DONE)
Status: **complete.** No compiler beats Apple clang 17; mimalloc gives ~+1%; 019's `q1`/`q2` (precomputed role ids) are the new best at ~4.0x.

## What was built (all from 019's candidate sources; no source change for z-builds)

Homebrew installs: mimalloc 3, LLVM/clang 23.1.2 (+ lld), GCC 16.2. Build script `tools/build_cc.sh` (whole-library PGO trained on golden-v1
development only, LTO where the toolchain supports it, tokenizer -O1 compiled once per compiler). `z1` = clang 23 + llvm-ar + ld64.lld; `z2` = GCC 16
(-flto=auto, profile-generate/use, no -g: the first GCC build spent >4 min per tool in dsymutil and was stopped); `z3` = Apple clang + mimalloc.o.
Earlier `x0`/`x1`/`x3` were built on 019 `b1` and verified but not timed (superseded by the z builds on `p1`). Build times: z1 221 s (incl. its own
tokenizer, 203 s), z2 120 s, z3 30 s (cached tokenizer). All builds byte-identical (dumps, frozen references, 2 x 2 M fuzz; `raw/verify_*`).

Mistakes recorded in `raw/compile_times.txt`: a build script edited while in use (x2, later killed); an over-broad `pkill` killed a build chain; a
leftover waiter released the timing pause during verification (contaminating 019 trial-1 runs, which were rerun); the x1 tokenizer cache key collided
(both compilers named `clang`), fixed for the z builds; `timeout` is not installed on macOS (z2's first attempt exited 127; rerun with a 30-min watchdog).

## Session 2 timing (AC, 03:57-05:02 PDT from start/DONE markers; `tools/run_timing.sh`, `raw/timing2/`): speedup vs the same-session original by median

| build | what | golden-v1 | novel | floor | golden-v3-osm | ASCII only |
|---|---|---:|---:|---:|---:|---:|
| 019 `p1` | reference | 3.818x | 3.820x | 3.785x | 3.786x | 3.787x |
| 019 `q1` | p1 + precomputed per-word role ids | **3.987x** | 3.975x | 3.941x | 3.970x | 3.947x |
| 019 `q2` | q1 + word memo + phrase canon cache | 3.970x | **3.997x** | **3.958x** | **3.975x** | **3.959x** |
| 019 `g1` | q2 + 2-byte and 3/4-byte phrase prefilters | 3.889x | 3.893x | 3.846x | 3.900x | 3.844x |
| 019 `g2` | g1 + pair features via the integer table | 3.853x | 3.849x | 3.841x | 3.921x | 3.802x |
| `z1` | p1, clang 23 | 3.774x | 3.782x | 3.759x | 3.725x | 3.727x |
| `z2` | p1, GCC 16 | 3.555x | 3.577x | 3.511x | 3.513x | 3.527x |
| `z3` | p1, Apple clang + mimalloc | 3.845x | 3.865x | 3.823x | 3.833x | 3.812x |

Original µs/address this session: floor 32.76, golden-v3-osm 21.42, ASCII 31.52; `q2`: floor 8.28, osm 5.39, ASCII 7.96.

## Decision

- Rejected: clang 23 (-1%), GCC 16 (-7%), the phrase prefilters in this pipeline (`g1` -2.5% vs `q2`; their table probes cost more than the skipped
  walks), pair routing (`g2`).
- Kept for the next build: mimalloc (+0.7-1.3% on `p1`; also lower peak memory).
- New best candidate: **019 `q2`** (≈ `q1`), ~4.0x on every dataset including the independent OSM set and the floor.
