# Deliverable `lf` (experiment 062, validated in 064) — how to rebuild

Validation (064, same session as the original, AC, sequential timing): golden-v1 **4.92×**, novel **4.86×**, OpenStreetMap **4.91×**. Output byte-identical to the original on golden-v1, novel, OSM, both frozen references and 2 × 2,000,000 fuzzed inputs; one thread; peak 2.537 GB vs 2.614 GB; steady 1,878 MB vs 2,492 MB (and vs the original's 2,172 MB state). PGO trained on golden-v1 development (031: training set does not bias speed). Load takes longer (≈10–11 s) because of load-time table and compression builds.

Changes over `final/048-va`, each exact (see the experiment records): 053 gap-encoded postcode-context graph (every row decoded and compared at load), 056 first-token table for the component-phrase trie (every vocabulary word's walk precomputed from the model; verified over 7.29M walks) and early role-row prefetch, 059/061 1-byte name ids in feature keys, 062 all-letters fast path in token normalization.

## Exact build used
```sh
cd research/experiments/062-letters-fastpath
EXTRA_SW="$(cat raw/switches.txt)" tools/build_cc.sh lf clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
`switches.txt` (copied here) lists every switch; `build_cc.sh` (copied here) compiles `sources/` over the pinned libpostal checkout with Apple clang 17, whole-library PGO + full LTO, mimalloc 3, tokenizer at -O1. `build_hashes.txt` = hashes recorded at build time.
