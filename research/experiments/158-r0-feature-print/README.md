# Experiment 158 — R0: repair cpi feature-print abort + setup_datadir API probe

Date/time/timezone: 2026-10-05T17:16:01-0700 (created by lead); work 17:17–19:12 PDT, America/Los_Angeles. Teammate r0.
Status: **correctness gate passed; timing gate open (TIMING_REQUEST.md). Separate re-setup defect in cpi found and reproduced (not fixed here).**

## Hypothesis and change

Defect (150): with `libpostal_parser_print_features(true)`, cpi exits 134 (`EXP_ROW_PACK: only the direct scoring path is supported`).

Data flow, traced before any fix:
1. `libpostal_parser_print_features(true)` → `address_parser_print_features` → `parser->options.print_features = true`.
2. `libpostal_parse_address` → `address_parser_parse` → `crf_tagger_predict(..., print_features)` → `crf_tagger_score_viterbi` → `crf_tagger_score`.
3. `crf_tagger_score`: the `EXP_DIRECT_KEYS` branch is skipped because of `!print_features`. It falls through to the `EXP_ROW_PACK` block: `fprintf` + `abort()` → SIGABRT → 134.
4. Removing the abort alone would not work. The fallback per-token loop loads `self->weights->indices->a` / `data->a`, and under `EXP_ROW_PACK` those are never allocated (`crf_sparse_read_packed` keeps only `indptr` + packed `{w, cls}` records). That would be a NULL dereference.
5. The rest of the fallback is valid in cpi:
   - With `crf_features_accumulate == 0`, the feature function emits plain strings. Int keys, role ids, const ids and CP ids are all gated on accumulate mode, and `fa_nb_arr` is NULL outside the direct pass, so no marker form is emitted.
   - `fd_lookup` routes string keys exactly like the load-time int split.
   - Viterbi depends only on the state/state_trans matrices.

Change (`source.patch`, crf.c only, +67/−1 lines): under `EXP_ROW_PACK`, the abort site now calls
`if (print_features) return crf_tagger_score_print(...)`. The abort is kept only for the remaining unsupported case, no dictionaries (`CRF_DICT_DISABLE`). The new function is `static __attribute__((noinline, cold))` and repeats the original per-token print and score loop, reading weights from `crf_pack_state` / `crf_pack_trans` (025's repair pattern). Same strings, same print format, same additions in the same order.

Base: `research/final/119-cpi/sources`, verified against its SHA256SUMS (`raw/base_source_check.txt`, all OK). Root checkout 181737d3 with untracked experiments. Candidate sources: `sources/`; unchanged copy: `sources_base/`.

## Reproduction

- Hardware/OS/compiler: Mac16,8, macOS 26.5.2, Apple clang 17.0.0 (clang-1700.6.3.2).
- Model: `/Users/kyle/.cache/libpostal-profile/libpostal`, same as every prior experiment. Not rechecked here; 150 checked it on 2026-10-05.
- Builds: `tools/build_cc.sh` (119's script with only `E=` retargeted) and the 119 switches (`raw/switches_cpi.txt`, equal to `final/119-cpi/switches.txt`). Both builds used PGO on golden-v1 development and ran through `model_slot.sh`. Commands, logs and exits are in `raw/build/`; both exited 0.
  - `158ctl` (`SRCDIR=sources_base`, unchanged) ran 17:19:08–17:24:04.
  - `158fp` (candidate) ran 17:24:03–17:24:41.
- 158ctl's binary hashes differ from 119's `build_hashes.txt` (libpostal.a 9cafd1… vs e56d3a…). This is expected: `-g` embeds the `artifacts/<name>/src` paths, and the profdata path differs. Its source hashes are equal, and its outputs equal 119's recorded outputs on every set (below).
- Probes: `tools/feat_dump.c` (003's dumper plus `print_features(true)`; prints `#ADDR`, the library's feature lines, then the parse) and `tools/api_probe2.c` (025's probe plus datadir and lifecycle modes). They were linked against `000-baseline/artifacts/libpostal.a` and against each candidate lib plus mimalloc, as 150 did (`raw/probe_builds.txt`).
- Artifact hashes: `raw/artifact_hashes.txt` and TIMING_REQUEST.md.

## All measured trials

None. Teammates do not time anything (team brief rule 5). See `TIMING_REQUEST.md`.

## Correctness and interpretation

**Feature-print stream** (`raw/feat/`, gz copies of every stream). Original vs 158fp `feat_dump`, `cmp` exit 0 on:

| Set | Addresses | Feature lines | SHA-256 (both) |
|---|---:|---:|---|
| golden-v1 development | 10,308 | 60,593 | 75867289… |
| 150 holdout (incl. 78 edge cases) | 3,298 | 22,927 | 090f75d7… |
| 150 DAD international | 45 | 351 | 871f40df… |

The parse lines produced in print mode match too. Negative control: flipping 1 byte makes `cmp` exit 1. 158ctl aborts (134) in this mode.

**API probe** (`raw/api/`, `compare.txt`). 9 modes × original / 158ctl / 158fp:
- Modes: `default`, `features`, `lifecycle`, `lifecycle_disable`, `datadir`, `datadir_features`, `datadir_lifecycle` (setup_parser_datadir ×4 with teardowns, printing on and then off), `datadir_bad` (missing parser dir returns 0, then the real dir loads), `lifecycle_long` (41-token address across a re-setup).
- 158fp stdout equals the original in all 9 modes. 158ctl exits 134 in `features`, `datadir_features` and `datadir_lifecycle`.
- The only stderr difference is in `datadir_bad`: the log macro prints `__FILE__:__LINE__` (for example `address_parser.c:215` vs the build path and line 311). The message text is equal, and 158ctl shows the same difference, so it is pre-existing.

**Full byte-identity suite** (`tools/verify.sh`, sequential, each run through `model_slot.sh`; `raw/verify_158fp/`, `raw/verify_158ctl/`). Both builds, all 15 runs exit 0, empty stderr:
- golden-v1 sha 45574ee2… and novel 806b8a81… (as expected);
- OSM v3, unseen v4–v11 and the 150 holdout and DAD: IDENTICAL to the original-library dumps;
- frozen golden-v1 and novel references: `RESULT: IDENTICAL`;
- fuzz seeds 1 and 2 × 2,000,000: IDENTICAL.

Reserved v12/v13 were not opened. Negative controls (`raw/negative_control/result.txt`): a mutated v1 dump gives cmp exit 1 and compare_frozen `mismatched=1 RESULT: DIFFERENT`; a mutated fuzz file gives cmp exit 1.

**Single thread.** The diff has no thread or parallel primitives (grep of `source.patch`). `ps -M` on the running 158fp `fuzz_pipeline` showed 1 thread in 6/6 samples at 92–100% CPU (`raw/thread_check.txt`). Attempt 1 sampled the bash wrapper's pid by mistake; it is kept as `thread_check_attempt1_wrong_pid.txt`.

**Hot path (disassembly, `raw/disasm/`).** `crf_tagger_score` is inlined into `_address_parser_parse`. The cold call site changed register allocation and spills there: 344 of about 3,317 normalized lines differ, and there are 12 more instructions. The print path itself is a separate 289-line function. Timing is therefore not structurally guaranteed to be unchanged and has to be measured: 158fp vs 158ctl.

## Separate finding: cpi is unsafe on parser teardown + re-setup (not fixed in 158)

The lead asked for one reproduction attempt (`raw/destroy_defect/`, logged in `raw/commands.txt`). The probe was `api_probe2` linked to the 158ctl lib with the **system** malloc (`artifacts/158ctl_sysmalloc`), because mimalloc ignores the Malloc* variables. It ran under `MallocGuardEdges=1 MallocScribble=1`.
- **Attempts 1–2:** stopped (external SIGTERM 143; then the user stop, 144). Partial output kept.
- **Attempt 3:** `lifecycle` and `lifecycle_long` both **segfault (139)** after the first parse and before the re-setup completes. The original under the identical probe exits 0 with stdout equal to its normal run (attempt 3c).
- **Attempt 3b (lldb):** `EXC_BAD_ACCESS` at a guard page in `cp_dfs` (119's `EXP_CP_IDS` load-time builder), on `str w8,[x0,x19,lsl #2]`, which is `cp_pool[cp_pool_n++] = id` inlined from `cp_ids_for`.
  - Source cause: `cp_build` shrinks `cp_pool` to `cp_pool_n` entries (`realloc`) but leaves `cp_pool_m` at the old capacity.
  - Nothing resets `cp_pool_n`/`cp_pool_m`/`cp_ready` or frees the tables at teardown.
  - So on re-setup the grow check is skipped and the build writes past the shrunken block. This is a deterministic heap overflow on every re-setup.
  - 150's lifecycle probe passed only because mimalloc's slack absorbed the write.
- **Attempt 3d (`CP_IDS_DISABLE=1`, isolating the rest):** still fails.
  - `lifecycle_long`: re-setup succeeds, then the second parse hits the `EXP_ROW_PACK` abort (134). The CRF dictionaries are absent after re-setup. Cause not traced.
  - `lifecycle`: segfault (139) before the second parse. Cause not traced.
- My earlier source-level hypothesis was that `crf_destroy` leaves `crf_lean_cap_items`, `crf_ik_active` and `crf_role_ids` stale, so a 10-row context would be treated as 32+ rows. That is **not isolated**: other faults occur first.
- Conclusion: **reproduced.** Re-setup after teardown is memory-unsafe in cpi, and therefore in 158fp, which shares the code. At least one cause is proven (CP_IDS pool); others remain. Single-setup use, which is what the contract and every benchmark exercise, is unaffected. The fix belongs to a later experiment numbered by the lead. A good first step is an ASan build of the lifecycle probe to enumerate every fault.

## What the print path reads (for 160 / P2)

`crf_tagger_score_print` reads the following:
- `crf_get_feature_id` → `fd_lookup((feature_dict_t *)self->state_dict, string)` (or `trie_get_data(self->state_features)` when the dict is NULL);
- `crf_get_state_trans_feature_id` → `fd_lookup(self->trans_dict)` (or the trie);
- `self->weights->indptr->a` and `self->state_trans_weights->indptr->a`;
- `crf_pack_state[col].{w,cls}` and `crf_pack_trans[col].{w,cls}`;
- `crf_feature_ids_reserve`;
- `self->trans_weights` (copied into `crf_context->trans` unless aliased);
- `state_score` / `state_trans_score_all` of `self->context`;
- the feature function called with `crf_features_accumulate == 0`, which must emit plain string features.

If P2 replaces the dictionaries or the packed rows, it must keep string-key lookup returning the same feature ids and keep row access by id. Otherwise feature printing breaks again. The 158 `feat_dump` comparison is the regression check.

## Decision

- R0 kill-rule items:
  - (1) outputs identical on all sets: **PASS**;
  - (3) both APIs behave like the original: **PASS** for print_features and setup_parser_datadir, including teardown/re-setup under the default mimalloc link. The re-setup memory-safety defect above is pre-existing in cpi and is reported separately.
  - (2) timing unchanged within noise: **OPEN**, pending the lead's run per TIMING_REQUEST.md (158fp vs 158ctl).
- Next action: lead timing; separate experiment for the re-setup defect.
- Ledger, report and activity log: not updated (read-only for teammates; the lead integrates).
