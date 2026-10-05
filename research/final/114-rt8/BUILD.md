# Deliverable `rt8` (experiment 114 on `psf`, validated in 116) — how to rebuild

Validation (116: cleaned machine, same session as the original, AC, sequential timing, 5 rotating rounds) gives golden-v1 **5.87×**, novel **5.77×** and OpenStreetMap **5.49×**. On unseen golden-v9 (115, never used before) it is **5.28×**, not slower than psf.
- **Output:** byte-identical to the original on golden-v1, novel, OSM, unseen golden-v4, v8 and v9, both frozen references and 2 × 2,000,000 fuzzed inputs.
- **Threads:** one.
- **Memory:** peak 2.543 GB vs 2.614 GB; steady 1,878 MB vs 2,492 MB.
- **Training data:** PGO trained on golden-v1 development.
- **Load time:** about 10 s.

Change over `final/101-psf`, **114 `EXP_ROW_TRANS_ONLY`:** the lookup stage prefetches every 128-byte line of each transition-weight row (up to 8), instead of only its first line. State rows keep the single-line hint. These are hints only: output cannot change. The saving is about 0.15 µs on longer, Senzing-style addresses; it is neutral on OSM-style ones.
Changes inherited from psf: 098 `EXP_NORM_PAIR` (one-scan standard + admin token normalization) and 101 `EXP_PS_FLAGS` (prefix/suffix "returns nothing" flags in spare FTT bits).

## Exact build used
```sh
cd research/experiments/114-trans-rows-prefetch
EXTRA_SW="$(cat raw/switches_rt8.txt)" tools/build_cc.sh rt8 clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
- `switches.txt` lists the extra switches.
- `build_cc.sh` compiles `sources/` over the pinned libpostal checkout. It uses Apple clang 17, whole-library PGO + full LTO, mimalloc 3, and the tokenizer at -O1.
- `build_hashes.txt` holds the hashes recorded at build time.
- Packaging caveats (from the handoff, still open): 091's backtrack should be guarded to aarch64 before any non-Apple-Silicon use; `build_cc.sh` has hard-coded local paths.
