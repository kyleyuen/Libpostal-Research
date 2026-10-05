# Deliverable `va` (experiment 048, validated in 051) — how to rebuild

`va` = the `l3` package (`final/021-l3`) plus two exact changes: `EXP_VITERBI20` (`crf_context.c`: the NEON Viterbi written out for exactly 20 labels with named registers; same operations in the same order per lane) and `EXP_TRANS_ALIAS` (`crf.c`: the per-address 3.2 KB copy of the transition matrix replaced by pointing at the model's matrix, restored before teardown).

Validation (051, same session as the original, AC, sequential timing): golden-v1 **4.53×**, novel **4.55×**, OpenStreetMap **4.62×** (`l3` 4.32× / 4.32× / 4.29× in the same session). Output byte-identical to the original on golden-v1, novel, OSM, both frozen references and 2 × 2,000,000 fuzzed inputs; one thread; peak 2.380 GB vs 2.614 GB; steady 2,141 MB vs 2,492 MB (and vs the original's 2,172 MB state seen earlier the same day). PGO is trained on golden-v1 development; experiment 031 showed the training set does not change speed on any dataset.

## Exact build used
```sh
cd research/experiments/048-viterbi20-alias
EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=3 -DEXP_VITERBI20 -DEXP_TRANS_ALIAS" \
  tools/build_cc.sh va clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
`build_cc.sh` (copied here) compiles the sources in `sources/` over the pinned libpostal checkout; base switches are listed inside it. Toolchain: Apple clang 17, whole-library PGO + full LTO, mimalloc 3 linked statically, tokenizer at -O1. `build_hashes.txt` = hashes recorded at build time.
