# Deliverable `pcp` (experiment 077, validated in 082) — how to rebuild

Validation (082, same session as the original, AC, sequential timing, 5 rotating rounds): golden-v1 **5.15×**, novel **5.07×**, OpenStreetMap **5.06×**; unseen golden-v5 (080, never used for tuning) **5.04×**. Output byte-identical to the original on golden-v1, novel, OSM, unseen golden-v4 and golden-v5, both frozen references and 2 × 2,000,000 fuzzed inputs (077 `raw/verify_pcp/summary.txt`, 080). One thread (six samples). Peak 2.543 GB vs 2.613 GB; steady 1,878 MB vs 2,492 MB. PGO trained on golden-v1 development (031: training set does not bias speed). Load ≈10–11 s.

Changes over `final/062-lf`, each exact: 067 per-address phrase-string cache with lengths passed to key building; 072 digit-only token fast path (exact; speed-neutral within noise per 073); 074 prefetch pre-pass for first-token table entries and their trie nodes (hint only; stage timers: context fill −0.14 µs); 077 prefetch of postcode-context rows after the postal-code search (hint only; postcode feature section −0.16 µs).

## Exact build used
```sh
cd research/experiments/077-pc-prefetch
EXTRA_SW="$(cat ../079-types-prefetch/raw/switches_pcp.txt)" tools/build_cc.sh pcp clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
`switches.txt` (copied here) lists the extra switches (the script's base list holds the rest); `build_cc.sh` compiles `sources/` over the pinned libpostal checkout with Apple clang 17, whole-library PGO + full LTO, mimalloc 3, tokenizer at -O1. `build_hashes.txt` = hashes recorded at build time.
