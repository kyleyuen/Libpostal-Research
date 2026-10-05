# Deliverable `vmx` (experiment 091, validated in 094) — how to rebuild

Validation (094, same session as the original, AC, sequential timing, 5 rotating rounds): golden-v1 **5.26×**, novel **5.19×**, OpenStreetMap **5.18×**; unseen golden-v7 (093, never used before) **5.22×**. Output byte-identical to the original on golden-v1, novel, OSM, unseen golden-v4 and golden-v7, both frozen references and 2 × 2,000,000 fuzzed inputs (091 checker builds `raw/verify_vmxv/`, 093). One thread. Peak 2.543 GB vs 2.613 GB; steady 1,878 MB vs 2,492 MB. PGO trained on golden-v1 development. Load ≈10–11 s.

Change over `final/077-pcp`: 091 — for the model's 20 labels the Viterbi forward pass keeps only the running maxima (hardware max instruction) and the backtrack recovers each step's argmax on the decoded path by recomputing the same scores in the same order and taking the first that equals the stored maximum (the original's first-strictly-greater rule).

## Exact build used
```sh
cd research/experiments/091-viterbi-maxonly
EXTRA_SW="$(cat raw/switches_pcp.txt) -DEXP_VITERBI_MAXONLY -DEXP_VITERBI_FMAX" tools/build_cc.sh vmx clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
`switches.txt` lists the extra switches; `build_cc.sh` compiles `sources/` over the pinned libpostal checkout with Apple clang 17, whole-library PGO + full LTO, mimalloc 3, tokenizer at -O1. `build_hashes.txt` = hashes recorded at build time.
