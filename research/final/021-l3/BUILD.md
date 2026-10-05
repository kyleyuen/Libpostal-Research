# Final deliverable `l3` (built in experiment 021, validated in experiment 032) — how to rebuild

Chosen 2026-10-02 to replace `l4` after experiment 031 found `l4`'s steady footprint 78 MB above the original's in a same-session check.
`l3` is `l4` with the feature dictionaries at load factor 1/3 instead of 1/4 (`EXP_LOADF=3`); sources are byte-identical to `final/021-l4/sources`.

Validation (experiment 032, 2026-10-02 15:13–15:26 PDT, AC, same session as the original):
- Speed vs the preserved original, 5 paired rounds: golden-v1 **4.38×**, novel **4.33×**, OpenStreetMap **4.28×** (`l4` in the same session 4.37× / 4.32× / 4.34×).
- Output: byte-identical to the original on golden-v1 (12,863), novel (12,863) and OSM (11,405), both frozen reference files, and 2 × 2,000,000 fuzzed inputs.
- One parsing thread (6 samples). Peak footprint 2.380 GB vs 2.614 GB; steady footprint 2,141 MB vs 2,172 MB (two samples each). Load time ~8.6 s vs ~2.9 s.
- PGO generality (experiment 031): PGO trained on golden-v1, novel or OSM gives the same speed on every dataset; the training set does not bias the result.

## Exact build used
```sh
cd research/experiments/021-final-stack
EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=3" \
  tools/build_cc.sh l3 clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
Base switches inside the script: EXP_NO_RESET EXP_DEDUP_BATCH EXP_FEATURE_DICT EXP_DIRECT_KEYS EXP_WEIGHT_PREFETCH EXP_LEAN_CONTEXT EXP_VOCAB_DICT
EXP_ASCII_NORM EXP_ADMIN_SHARE EXP_FAST_APPEND EXP_FAST_JOIN EXP_VOCAB_MEMO EXP_INT_KEYS EXP_TRANS_SKIP EXP_NORM_TABLE EXP_CANON_CONST
EXP_RESPONSE_BLOCK EXP_REUSE EXP_VITERBI_NEON EXP_TOKEN_ASCII EXP_FD_INLINE.
Toolchain: Apple clang 17.0.0 (whole-library PGO + full LTO; PGO trained on `profiling/golden-v1/development.txt`), mimalloc 3 (Homebrew) linked
statically via `mimalloc.o`, tokenizer `scanner.c` at -O1. Model files: `profiling/manifest.json`. `build_hashes.txt` = the hashes recorded when 021 built `l3`
(032 confirmed the binaries still match them).
