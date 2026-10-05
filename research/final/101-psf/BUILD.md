# Deliverable `psf` (experiments 098 + 101 on `vmx`, validated in 103) — how to rebuild

Validation (103: same session as the original, AC, sequential timing, 5 rotating rounds) gives golden-v1 **5.44×**, novel **5.39×** and OpenStreetMap **5.45×**. On unseen golden-v8 (102, never used before) it is **5.37×**.
- **Output:** byte-identical to the original on golden-v1, novel, OSM, unseen golden-v4 and golden-v8, both frozen references and 2 × 2,000,000 fuzzed inputs (098/101 `raw/verify_*`, 102).
- **Threads:** one.
- **Memory:** peak 2.543 GB vs 2.614 GB; steady 1,878 MB vs 2,492 MB.
- **Training data:** PGO trained on golden-v1 development.
- **Load time:** about 10 s (+0.2 s over vmx for 101's load-time pass).

Changes over `final/091-vmx`:
- **098 `EXP_NORM_PAIR`:** each token's standard and admin normalizations come from one scan instead of two. With the parser's two option sets, only digit replacement differs between them. Tested on 24.6M cases with 0 mismatches.
- **101 `EXP_PS_FLAGS`:** at load, the address-dictionary prefix and suffix searches run once on every vocabulary word. Two spare bits of the existing first-token-table entry (056) record "returns nothing". At parse time, a token whose raw bytes equal its admin form and that has a table slot skips each flagged search. Flags come from the model only; no extra memory. 30M skipped searches were re-verified.

## Exact build used
```sh
cd research/experiments/101-prefix-suffix-flags
EXTRA_SW="$(cat raw/switches_psf.txt)" tools/build_cc.sh psf clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
- `switches.txt` lists the extra switches.
- `build_cc.sh` compiles `sources/` over the pinned libpostal checkout. It uses Apple clang 17, whole-library PGO + full LTO, mimalloc 3, and the tokenizer at -O1.
- `build_hashes.txt` holds the hashes recorded at build time.
- Packaging caveats (from the handoff, still open): 091's backtrack should be guarded to aarch64 before any non-Apple-Silicon use; `build_cc.sh` has hard-coded local paths.
