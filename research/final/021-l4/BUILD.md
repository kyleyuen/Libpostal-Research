# Final candidate `l4` (experiment 021) — how to rebuild

Result (experiment 021, AC, same-session original): golden-v1 **4.33x**, novel **4.36x**, floor (non-ASCII) **4.27x**, independent OpenStreetMap set
**4.31x**, ASCII-only **4.30x**. Byte-identical to the original on golden-v1, novel, golden-v3-osm, the frozen references and 2 x 2,000,000 fuzzed inputs;
one thread; peak footprint 2.471 GB vs 2.613 GB, steady 2,250 MB vs 2,492 MB. Load time 8.7 s (original 3.4 s; `experiments/021-final-stack/raw/load_time_l4.txt`); full build ~5 min (tokenizer).

## Contents
- `sources/`: the 11 changed libpostal sources and headers exactly as compiled (from `experiments/019-fixed-costs/candidate/`); all other libpostal
  files come unchanged from `libpostal/src/` (pinned checkout).
- `build_cc.sh`: the build script used (copy of `experiments/021-final-stack/tools/build_cc.sh`).
- `build_hashes.txt`: hashes of the sources, library and binaries of the measured build; `SHA256SUMS`: hashes of this folder.

## Exact build used
```sh
cd research/experiments/021-final-stack
EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=4" \
  tools/build_cc.sh l4 clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```
Base switches inside the script: EXP_NO_RESET EXP_DEDUP_BATCH EXP_FEATURE_DICT EXP_DIRECT_KEYS EXP_WEIGHT_PREFETCH EXP_LEAN_CONTEXT EXP_VOCAB_DICT
EXP_ASCII_NORM EXP_ADMIN_SHARE EXP_FAST_APPEND EXP_FAST_JOIN EXP_VOCAB_MEMO EXP_INT_KEYS EXP_TRANS_SKIP EXP_NORM_TABLE EXP_CANON_CONST
EXP_RESPONSE_BLOCK EXP_REUSE EXP_VITERBI_NEON EXP_TOKEN_ASCII EXP_FD_INLINE.
Toolchain: Apple clang 17.0.0 (whole-library PGO + full LTO; the PGO profile is trained by running the instrumented dumper over
`profiling/golden-v1/development.txt` only), mimalloc 3 (Homebrew) linked statically via `mimalloc.o`, tokenizer `scanner.c` at -O1 (upstream -O0).
The model files are those in `profiling/manifest.json` (`~/.cache/libpostal-profile/libpostal`).
Every switch, its exactness argument and its measured effect are in the experiment READMEs 003-021 (ledger: `experiments/README.md`).
