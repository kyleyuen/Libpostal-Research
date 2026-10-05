# Deliverable `cpi` (experiment 119 on `rt8`, validated in 121) — how to rebuild

Validation (121: cleaned machine, same session as the original, AC, sequential timing, 5 rotating rounds) gives golden-v1 **6.08×**, novel **5.93×** and OpenStreetMap **5.78×**. On unseen golden-v10 (120, never used before) it is **5.68×**.
- **Output:** byte-identical to the original on golden-v1, novel, OSM, unseen golden-v4, v8, v9 and v10, both frozen references and 2 × 2,000,000 fuzzed inputs.
- **Threads:** one.
- **Memory:** peak 2.543 GB vs 2.614 GB; steady 1,927 MB vs 2,492 MB.
- **Training data:** PGO trained on golden-v1 development.
- **Load time:** about 12 s (+2.2 s for the 119 table).

Change over `final/114-rt8`, **119 `EXP_CP_IDS`:**
- **Load.** Every key of the component-phrase trie (4.01M; city, suburb, state and other names from the model) is enumerated. The component-phrase features its data id would produce are looked up once, and their feature ids stored (47.8 MB).
- **Parse.** When the matched phrase string has the key's length, which proves it equals the key, those ids are used directly instead of building and looking up strings.
- **Verification.** Every use was re-verified against the string path on all sets and 4M fuzz inputs.
Inherited: 114 (transition-row prefetch), 098, 101 and everything in earlier packages.

## Exact build used
```sh
cd research/experiments/119-component-phrase-ids
EXTRA_SW="$(cat raw/switches_cpi.txt)" tools/build_cc.sh cpi clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
- `switches.txt` lists the extra switches.
- `build_cc.sh` compiles `sources/` over the pinned libpostal checkout. It uses Apple clang 17, whole-library PGO + full LTO, mimalloc 3, and the tokenizer at -O1.
- `build_hashes.txt` holds the hashes recorded at build time.
- Packaging caveats (from the handoff, still open): 091's backtrack should be guarded to aarch64 before any non-Apple-Silicon use; `build_cc.sh` has hard-coded local paths.
