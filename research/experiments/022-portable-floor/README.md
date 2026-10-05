# Experiment 022 — Portable lower bound of the final candidate

Date/time/timezone: build 2026-10-01T09:05:17 to 2026-10-01T09:05:23 PDT (from raw/build_*.txt)
Status: **complete.** Speed: `pt` 3.74-3.86x (portable part of the gain). Memory: **`pt` FAILS the no-increase rule** (peak 3.176 GB, steady 3,029 MB vs the original's 2.614 GB / 2,492 MB) — the memory result of `l4` depends on mimalloc.

## Hypothesis and change

How much of 021 `l4`'s ~4.3x does not depend on this machine/toolchain? `pt` = exactly l4's sources and switches, minus everything machine- or
toolchain-specific: no ARM NEON Viterbi (`EXP_VITERBI_NEON` removed; the same scalar path an x86 build compiles, since the NEON code is also guarded by
`__aarch64__`), no profile-guided optimization, no link-time optimization, no mimalloc (system allocator). Plain `-O2`; tokenizer at `-O1` (a
plain compiler flag; upstream uses -O0). Still Apple clang on arm64 — this approximates, but does not replace, a run on x86/Windows. Prediction: none
(the 3.5-3.8x mentioned in conversation is an UNVERIFIED estimate).

## Command
```sh
cd research/experiments/022-portable-floor
EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=4" \
  tools/build_cc.sh pt clang none "-O2" "-O1" ""
```
Exit 0, no errors (`raw/build_pt.log`); 6 s with the cached tokenizer object. Hashes and exact flags: `artifacts/pt/hashes.txt`.


## Verification (11:15-11:17 PDT, `raw/verify_pt/`)

Byte-identical: golden-v1 and novel dumps (same SHA-256 as the original), both frozen references IDENTICAL, fuzz seeds 1 and 2 IDENTICAL.

## Results (AC, 11:17-11:38 PDT from raw/timing/start.txt and DONE; `raw/timing/`): speedup vs the same-session original by median

| build | golden-v1 | novel | floor | golden-v3-osm | ASCII | peak footprint |
|---|---:|---:|---:|---:|---:|---:|
| original | 34.03 µs | 34.98 µs | 33.08 µs | 21.75 µs | 32.00 µs | 2.614 GB |
| 021 `l4` | 4.308x | 4.406x | 4.344x | 4.365x | 4.348x | 2.471 GB |
| **`pt`** | **3.770x** | **3.858x** | **3.767x** | **3.743x** | **3.798x** | **3.176 GB** |

`pt` memory over time (`raw/footprint_trace_pt.txt`): climbs during load to 3,028 MB at 10 s and stays at 3,029 MB while parsing — a steady
increase, not a load spike. `l4` steady is 2,250 MB (021). So the same tables cost ~780 MB more under the macOS system allocator than under mimalloc.
Cause not yet diagnosed (UNVERIFIED candidates: freed load-time structures not returned to the OS by the system allocator, or allocation-size rounding).

## Decision

- Speed: the toolchain-independent part of the work gives **3.74-3.86x** (≈ 87% of `l4`'s speed); NEON + PGO + LTO + mimalloc add the rest (~15%).
  This is still on arm64 Apple clang, so it is a proxy for, not a measurement of, other machines.
- Memory: `pt` is **rejected** as a deliverable. mimalloc is not optional for the memory rule. It is itself portable (macOS, Linux, Windows), so the
  next portable candidate is `pt` + mimalloc, and its memory must be measured; the cause of the 780 MB gap should be diagnosed first.
