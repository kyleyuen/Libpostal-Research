# Accepted CPI methods: source and build support

These are verified implementation facts to use in the detailed outcome report. CPI is a cumulative package; do not present every feature as work added by experiment 119 alone.

## Runtime methods enabled in the accepted package

`research/final/119-cpi/switches.txt` lists the incremental switches; `research/experiments/119-component-phrase-ids/tools/build_cc.sh` supplies the base switch set.

- **Dictionary lookups and integer feature IDs:** base switches include `EXP_FEATURE_DICT`, `EXP_DIRECT_KEYS`, `EXP_VOCAB_DICT`, and `EXP_INT_KEYS`; package additions include `EXP_CP_IDS`. Relevant implementation: `research/experiments/119-component-phrase-ids/sources/feature_dict.h`, `crf.c`, and `address_parser.c`.
- **Reuse of address-local text and tokens:** enabled flags include `EXP_VOCAB_MEMO`, `EXP_WORD_MEMO`, `EXP_WM_CANON`, and `EXP_REUSE`; implementation is in `address_parser.c`. Describe this as within-address reuse, not cross-address caching.
- **Model-derived tables:** `EXP_FTT` and `EXP_PC_COMPRESS` are enabled in the package. `EXP_CP_IDS` enumerates the model's component-phrase trie and stores the corresponding state-feature IDs. `research/experiments/119-component-phrase-ids/README.md` records 4,012,750 trie keys, 6,519,618 emitted IDs, a 47.8 MB table, and the exact-key/length check used at parse time.
- **String and normalization work:** enabled flags include `EXP_ASCII_NORM`, `EXP_NORM_TABLE`, `EXP_NORM_PAIR`, `EXP_FAST_APPEND`, `EXP_FAST_JOIN`, `EXP_NAME_BYTE`, `EXP_LETTERS_FAST`, and `EXP_DIGITS_FAST`. Do not call every candidate optimization a separate string copy or heap allocation without matching source counters.
- **Scoring and decoding:** enabled flags include `EXP_VITERBI20`, `EXP_VITERBI_MAXONLY`, and `EXP_VITERBI_NEON`. `crf_context.c` gates the NEON path on `__aarch64__`; it vectorizes work within the parser thread. Do not describe this as multithreaded parsing.
- **Prefetch and layout:** `EXP_WEIGHT_PREFETCH`, `EXP_FTT_ROLE_PREFETCH`, `EXP_PC_PREFETCH`, `EXP_ROW_PACK`, and related row settings are enabled. Describe these as memory-layout/access choices, not model changes.

The switch list contains no OpenMP or worker-pool flag. The actual one-thread claim comes from the final run's thread samples, not from flag absence alone.

## Accepted package build evidence

- Exact package rebuild command and summary: `research/final/119-cpi/BUILD.md`.
- Build identity and switches: `research/final/119-cpi/build_hashes.txt` and `switches.txt`; static archive SHA-256 `e56d3ae4799966543a5841ebce89f26fb5eeed396d862f6b31e4076354675a4e`.
- Recorded recipe: Apple Clang, whole-library O2, `-flto`, scanner O1, Clang/LLVM profile-guided optimization, and `/opt/homebrew/opt/mimalloc/lib/mimalloc.o` at link. The package record describes PGO as trained on `profiling/golden-v1/development.txt`.
- PGO mechanics: `research/experiments/119-component-phrase-ids/tools/build_cc.sh` lines 27–35 build an instrumented executable, run its dumper on `profiling/golden-v1/development.txt`, merge `.profraw` files to `dev.profdata`, and rebuild with `-fprofile-instr-use`. The exact invocations are in `research/experiments/119-component-phrase-ids/raw/commands.txt`; compile profile and flags are in `raw/compile_times.txt` and `research/final/119-cpi/build_hashes.txt`.
- Cumulative scope: `research/final/119-cpi/BUILD.md` says this adds `EXP_CP_IDS` over the previous 114 package and inherits earlier work. The experiment 119 README explains that incremental change; the current report should summarize the combined package without attributing all inherited work to 119.

## Reporting limits

The above sources establish which paths/flags were built, not their individual speed contribution in the final package. Use the new 202 timing and memory report for measured results. Keep finite output checks and API scope explicit; never claim all strings or all public API states.
