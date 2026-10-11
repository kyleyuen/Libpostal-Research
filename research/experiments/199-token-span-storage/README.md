# Experiment 199 — token-span storage

Date/time/timezone: started 2026-10-07 19:11 PDT, America/Los_Angeles.  
Status: development/golden, all13 output/thread, 200,000-input exact-output fuzz, and 78 safe public-API cases passed. The allocator-only mimalloc-v2 artifact passed the all13 peak-footprint and golden steady-footprint gates. Uninstrumented speed was not measured; the separate ctypes API harness recorded a higher peak than O000 and prevents a blanket lower-memory claim.  
Evidence type: source-backed architecture implementation with parser, correctness, operation, and memory evidence; no speed claim.

## Hypothesis and change

The parser copies each retained scanner token into a `cstring_array`, even though token offset and byte length already point into the owned normalized input string. This candidate stores retained tokens as offset/length/type spans into one owned normalized-string copy, inserting NUL terminators only at safe gaps. It keeps the standard scanner, context fill, normalization, phrase searches, feature occurrences, and scoring order. If retained token spans touch or overlap, the original per-token copy path is used. The eligibility check depends only on this address’s scanner spans, not on country, corpus, or token content. It adds no cache and does not remove feature multiplicity.

Source changes are in `sources/`; exact patch and manifest are `raw/source.patch` and `raw/source-manifest.sha256`. The implementation was copied from experiment 119’s source snapshot (base commit `181737d30977adc9d287e3f7a1cfafd7277525d4`). The release switches are the recorded 119/CPI set plus `-DEXP_TOKEN_SLICES`; no experiment-198 certified-Viterbi switch is present. Build flags: clang library `-O2 -flto`, scanner `-O0`, link `-flto`, no PGO. The unchanged generated `scanner.c` object was reused from experiment 198’s build with matching clang `-O0`; source/object hashes and provenance are in `raw/handoff.json`.

## Operation diagnostics

For eligible addresses, this replaces copying each retained token's bytes into the growable C-string token array with one offset/length/type record and NUL writes at safe delimiter gaps in the already-owned normalized buffer. This description concerns copied token bytes and storage entries; it does not assume that the original array performs one heap request per token. Added work is one eligibility scan and slice-mode token access. Addresses with unsafe spans use the old copy path. Corrected diagnostic counters and output preservation are recorded in `REPORT.md`. Release peak-footprint evidence is now available for system-allocator v1 (fails) and allocator-only mimalloc v2 (passes the 13-workload matched comparison); steady-state capacity/memory and uninstrumented speed remain unmeasured.

## Build history

All attempts and command output remain under `raw/`:

- `failed-build-1/`: compile failed because included upstream headers resolved the old token declarations.
- `failed-build-2/`: same include-resolution issue remained after first attempted header override.
- `failed-build-3/`: source compilation passed; build failed because the local `artifacts/scanner_cache` directory was absent.
- `failed-build-4/`: operator-stopped after 3m50s compiling the unchanged generated scanner at `-O1`; clang was still using 100% CPU and had produced a zero-byte temporary. No compiler exit code was emitted. This is recorded as termination, not a compiler failure.
- `preliminary-with-198-cert/`: successful preliminary build accidentally included experiment 198’s certifier switch. Its source and logs are retained but it is not the candidate handed off.
- `raw/build.*`: final candidate build, exit 0. It includes only the 119/CPI switches and `-DEXP_TOKEN_SLICES`.

The original O2 release used scanner O0 and the system allocator; its all13 peak was higher than O000. To isolate that profile confound, a separate `release-mimalloc-v2` artifact was relinked from the unchanged 199 archive with the canonical static mimalloc object and `-flto`. It remains scanner O0 and has no PGO, so it is an allocator-only profile correction rather than the accepted full 119 profile. The archive hash remained unchanged. Exact argv, input hashes, timestamps, exits and output hashes are in `raw/mimalloc-relink-v2/manifest.json`.

## Verification and memory evidence

The mimalloc-v2 build passed development, golden and all 13 observed-workload output/thread checks against O000. The golden stdout hash is `45574ee2b1972f1acf2dbf53eb1fc2c25f8e1972cd5c22e8071fe3606c5c8c63`; the standalone frozen-reference comparator reports 12,863 rows, zero missing and zero mismatched. Every parser role observed one thread. The all13 raw batch is `raw/mimalloc-all13-v2/`.

In that same all13 batch, maximum `/usr/bin/time -l` `peak_footprint_bytes` was 2,611,677,416 B for O000 and 2,517,404,024 B for 199 mimalloc-v2 (94,273,392 B lower). This is one all13 screen, not a repeated timing campaign. The accepted steady-footprint procedure on golden-v1, after three warm passes, reported 2,156 MB for O000 and 1,927 MB for 199 mimalloc-v2 at the suite's rounded precision. Six thread samples per process all reported one. Records are under `research/suite/results/parallel-20261008-199-mimalloc-v2-memory/`.

The deterministic fuzz stream used seed 1 and 200,000 attempted inputs from golden-v1 plus golden-v2 novel. A separate exact serializer records each generated input as length-prefixed hex along with status and every ordered label/component byte string; the original and candidate streams were byte-identical (57,956,637 bytes, SHA-256 `d9331f364b6ed77c56e3bb695599c308162da54ed13442384c7f69f155d73030`). There were 178,955 valid UTF-8 parses and 21,045 invalid UTF-8 inputs explicitly recorded and skipped because the original has undefined/crash behavior for them; no empty or NULL cases occurred. This lane compares outputs only and is not timed evidence. Raw record: `raw/fuzz-exact-200k-seed1-mimalloc-v2/`.

The direct public-API harness passed all 78 safe registry rows against O000 with identical ordered JSONL bytes (SHA-256 `bfcd44cb99eabc2d9fc86cb0dc60816a8877b932595e434180494314ad1fb665`) and one observed thread. Rows cover empty strings, options, embedded NUL behavior, and the explicit NULL pointer probe; four malformed-byte records remain excluded. The ad-hoc Python/ctypes harness peak footprints were 2,301,265,960 B for O000, 2,622,114,072 B for 197 and 2,530,331,120 B for 199 mimalloc-v2. Candidate shared libraries were linked from static release archives while O000 used its preserved dylib, so these peaks are not a matched product memory comparison; they nevertheless prevent a blanket claim that candidate memory is always lower. API records: `research/analysis/parallel-exploration-2026-10-07/raw/api-edge-78-finalists-v1/`.

## Decision

The v2 allocator-only artifact passes the measured all13 output/thread and peak-footprint screens, the suite's rounded golden steady-footprint gate, the exact 200,000-input lane on its valid-UTF-8 subset, and the 78 safe public-API output contract cases. The API harness peak is higher than O000 under an unmatched shared-library link setup, so no blanket memory reduction claim is made. The full accepted PGO/scanner profile was not built, and faster uninstrumented execution was not measured; do not promote the architecture on performance claims from this evidence.
