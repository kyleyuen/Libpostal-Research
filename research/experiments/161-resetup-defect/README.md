# Experiment 161 — fix cpi teardown→re-setup memory defect

Date/time/timezone: 2026-10-05T19:15:24-0700 (created by lead); work 19:16–20:08 PDT, America/Los_Angeles. Teammate r0.
Status: **kill-rule items 1–3 PASS; item 4 (timing vs 158fp) OPEN → TIMING_REQUEST.md.**

## Hypothesis and change

158 reproduced it: after `libpostal_teardown_parser` → re-setup, cpi is memory-unsafe (guard malloc segfault). Hypothesis: setup-derived file-scope
state survives teardown. Root causes, found by ASan on an unfixed build (161asan0b = 158fp sources):

| # | Fault (ASan, `raw/asan0b*/`) | Root cause |
|---|---|---|
| 1 | heap-buffer-overflow WRITE 4 B, 0 bytes past a 26,078,472-byte block, `cp_ids_for` address_parser.c:1527 ← `cp_dfs` ← `cp_build` ← `address_parser_load` (re-setup) | `cp_build` shrinks `cp_pool` to `cp_pool_n` with realloc but keeps `cp_pool_m` at the old capacity; nothing resets `cp_pool_n/m` at teardown, so the second build skips the grow and writes past the shrunk pool |
| 2 | with `CP_IDS_DISABLE=1`: heap-buffer-overflow WRITE 2240 B / 6560 B (memset) into a 1600-byte block, `crf_context_set_num_items_lean` crf.c:747 ← `crf_tagger_score` (first parse after re-setup) | `crf_lean_cap_items` survives teardown (32 or 64), but the re-setup context is new with `CRF_CONTEXT_DEFAULT_NUM_ITEMS` = 10 rows (10×20×8 = 1600 B); the resize is skipped and T×L doubles are written |

The 158 `CP_IDS_DISABLE` failures (ROW_PACK abort with "dictionaries absent" on the second parse, and a segfault) were **consequences of fault 2**.
The heap corruption came from the over-long memsets; the lead's "stale dictionary globals" hypothesis was **not** the cause. After the fix, those
modes pass with 0 ASan reports (`raw/asan_fx_cpoff`). A single setup is clean under ASan both before and after the fix (`default`, 0 reports).

Change (`source.patch`; crf.c +47, address_parser.c +28/−1, transliterate.c +4; teardown and setup-failure code only):
- crf.c `crf_statics_reset()`, called at the end of `crf_destroy`, resets:
  - `crf_lean_cap_items` = 0;
  - `crf_ik_active` = 0;
  - frees `crf_ik_keys[2]` and zeroes `crf_ik_n/m`;
  - `crf_ik_name` = −1;
  - frees `crf_role_ids` (leaked before; it kept answering after its dictionaries were freed);
  - `crf_const_ready` = 0 and `crf_const_ids` = NO_FEATURE.

  This is a port of 025's `EXP_AUDIT_FIXES` block plus `crf_ik_name` and `crf_const_ids`.
- crf.c `crf_dict_retire()` replaces `fd_destroy` for the state and transition dictionaries in `crf_destroy`. It frees the contents and keeps
  the header. Reason: the per-call-site `fa_nb_cache_t` name caches (address_parser.c:1437/1445/1446 macros) are keyed by dictionary address.
  A later setup's dictionary at a reused address would hit a stale name id if that setup loads a different model. With the same model the ids
  are equal, so this cannot change output in the probes. Keeping the header makes addresses unique without touching `feature_array_add_nb` (parse
  path). Cost: 2 headers (about 7 KB) leaked per teardown, marked with a `ponytail:` comment.
- address_parser.c `ap_statics_reset()`, called by `address_parser_destroy` for the module's parser:
  - frees `cp_start/cnt/len/pool/tmp` and zeroes `cp_n/pool_n/pool_m`, with `cp_ready`=false and `cp_dict`=NULL;
  - frees `trie_ftt_tab` (leaked before) and clears `trie_ftt_trie/slot/n`;
  - resets `ap_canon_unknown*` and `ap_canon_const_ready`;
  - frees `nt_table` and sets `nt_state`=0;
  - zeroes `pc_rows/m/n`.

  `cp_build`'s `cp_n == 0` failure path now NULLs what it frees, so there is no double free at teardown.
- transliterate.c `transliteration_module_teardown`: resets the `ts_*` skip facts (`ts_state`, counters, bitmap, scan state), which were derived
  from the table it destroys.

Base: 158fp sources (`158-r0-feature-print/sources`), equal to `158fp/src` (`raw/base_source_hashes.txt`). Unchanged copy: `sources_base/`.
Root checkout 181737d3 plus untracked experiments.

## Item 2: inventory of every file-scope / static state in the changed sources (line numbers in 161 `sources/`)

Compiled = under `switches_cpi.txt` + `build_cc.sh` base switches.

**Completeness check (not regex-based):** `raw/inventory_nm.txt` holds every `__DATA` (data/bss/common) symbol of the 11 changed objects of the
non-LTO `161asan` build (same switches), from `nm -m`. Every symbol there is classified below.
- Items missing from the first source grep and added from nm:
  - `norm_table_test_disable` and `trans_skip_test_disable`: test switches, 0 in normal use, never set;
  - `DEFAULT_ALPHABET` (trie.c): constant;
  - `ap_pm_gen`: per-address generation counter;
  - `ts_scan.path` (= `path[520]`).
- The `fa_nbc_` caches are **42** call-site statics, one per use of the 3 macros (`_address_parser_features.fa_nbc_.*`, `_ap_cp_emit.*`,
  `_add_phrase_features_l.*`, `_add_ngram_features.*`).
- Absent from the objects, which confirms they are not compiled or are eliminated: `tpf_cache`, `ci0/cis`, `crf_st_*`, `crf_weights_finite`,
  the VERIFY/STATS counters and `crf_state_batch/trans_batch`. The last are dead under ROW_PACK; the batched path is unreachable.
- Also absent: `ts_plen`, presumably optimized into registers. It is reset anyway.
- `ltmp*` entries are assembler section labels, not variables.

**Model-derived, reached at setup — now reset or freed at teardown:**

| State | file:line | Set by | Teardown (161) |
|---|---|---|---|
| `cp_start, cp_pool, cp_cnt, cp_len, cp_n, cp_pool_n, cp_pool_m, cp_ready` | address_parser.c:1521 | `cp_build` (setup) | freed/zeroed, `ap_statics_reset` (**fault 1**) |
| `cp_tmp, cp_dict` | address_parser.c:1522 | `cp_build` | destroyed/NULL |
| `crf_lean_cap_items` | crf.c:730 | first parse | 0, `crf_statics_reset` (**fault 2**) |
| `crf_ik_active` | crf.c:330 | `crf_int_keys_enable` | 0 |
| `crf_role_ids` | crf.c:333 | `crf_int_keys_enable` | freed/NULL (was leaked and stale) |
| `crf_const_ids[2], crf_const_ready` | crf.c:337–338 | `crf_int_keys_enable` | NO_FEATURE / 0 |
| `crf_ik_name[2][6]` | crf.c:340 | `crf_int_keys_enable` | −1 |
| `crf_ik_keys[2], crf_ik_n, crf_ik_m` | crf.c:341–342 | per address (int keys) | freed/0 |
| `trie_ftt_tab, trie_ftt_trie` (+ `trie_ftt_slot/n`, per address) | trie_search.c:344 | `ap_ftt_build` | tab freed (was leaked), pointers NULL |
| `ap_canon_unknown, ap_canon_unknown_numeric, ap_canon_const_ready` | address_parser.c:27–28 | `address_parser_load` | reset |
| `nt_table, nt_state` | address_parser.c:541 (+19) | `nt_init` (setup) | freed / 0 (rebuilt next setup) |
| `ts_state, ts_cp_bitmap, ts_n_amp, ts_n_cp, ts_prefix/plen/ok/mode` | transliterate.c:675–678 | `ts_init` (reached at parser setup via `nt_init`) | reset in `transliteration_module_teardown` |
| `state_dict, trans_dict` addresses as keys of the 42 `fa_nb_cache_t` call-site statics (macros at address_parser.c:1437, 1445, 1446) | — | parse | headers retired, so addresses are never reused |

**Model-derived, already handled before 161 (verified):**
- `vocab_dict` (address_parser.c:16): `fd_destroy` + NULL in `address_parser_destroy`.
- `pc_bytes, pc_off, pc_on` (address_parser.c:181): freed in destroy; 161 also zeroes `pc_rows/m/n`.
- `crf_pack_state, crf_pack_trans` (crf.c:12): freed + NULL in `crf_destroy`.
- `crf_trans_saved` (crf.c:572): restored and NULL in `crf_destroy`.
- `parser` (address_parser.c:39): NULL in `address_parser_module_teardown`.
- `trans_table` (transliterate.c:16): NULL in its teardown.

**Per-address scratch: reached at parse, holds no model data, keeps capacity across setups (unchanged):**
`ap_wm` (1343), `ap_pm` (1458), `ap_vm` (1666), `rs_*` (2437), `ftt_slots` (1099), `cp_buf` (1537, setup scratch), `crf_feature_ids` (crf.c:75),
`crf_state_batch/trans_batch` (108), `crf_state_ids/trans_ids/state_tok/trans_tok` (560), `crf_features_accumulate` (326),
`fa_nb_dict/fa_nb_arr` (features.c:68, set and cleared per scoring pass), `vmo_max/vmo_cap` (crf_context.c:575), `path[520]` (transliterate.c:713).

**Pure function of compiled-in tables:** `nt_cat, nt_hy, nt_ready` (normalize.c:396, utf8proc categories).

**Not compiled in cpi:**
- `ci0/cis` (address_parser.c:1126, `EXP_FTT_PF3`);
- `crf_weights_finite` (crf.c:16, `EXP_BACK_NOCLEAR`; reset anyway under its guard);
- `crf_st_*` (crf.c:573, `EXP_ALLOC_LEAN`);
- `crf_ik_verify_str, crf_ik_full` (crf.c:344–345, `EXP_INT_KEYS_VERIFY`);
- `tpf_cache/tpf_n` (trie_search.c:254, `EXP_PHRASE_PREFILTER`);
- `ps_verified, cp_verified, ds_*`, `trie_ftt_verified`, `tpf_verified` (VERIFY/STATS switches).

## Reproduction

- Mac16,8, macOS 26.5.2. 161fx: Apple clang 17.0.0 (same recipe as cpi/158).
- **ASan: Homebrew clang 23.1.2.** Apple clang 17's ASan runtime hangs before `main` on this OS, in `__asan::InitializeShadowMemory` →
  `get_dyld_hdr` → `dyld_shared_cache_iterate_text_swift`. Attempt 1 (`161asan0`, `raw/asan0`) was killed after about 10 min. A 4-byte
  heap overflow test hangs with Apple clang and is caught with clang 23 (exit 134, 1 report).
- Builds (`raw/build/*.cmd|stdout|stderr|exit`, all exit 0):
  - `161asan0` (Apple, unusable) 19:16:55–19:21:44;
  - `161asan0b` (pre-fix, clang 23, `-O1 -fsanitize=address`, no PGO) 19:33:09–19:36:36;
  - `161asan` (fix) 19:42:51–19:42:57;
  - `161fx` (`tools/build_cc.sh` = 119's script with `E=` retargeted, 119 switches, PGO dev, `-O2 -flto`, mimalloc) 19:51:06–19:51:41.
- Probes: 158's `api_probe2` (9 modes) linked three ways:
  - mimalloc (`artifacts/161fx`);
  - system malloc (`artifacts/161fx_sysmalloc`; control `artifacts/158fp_sysmalloc`);
  - ASan.
- Runners: `tools/probe_run.sh` and `tools/asan_run.sh`, one model process at a time via `model_slot.sh`. Hashes: `raw/artifact_hashes.txt`.

## All measured trials

None (team brief rule 5). See TIMING_REQUEST.md.

## Correctness and interpretation

**Item 1** (stdout compared with 158's `raw/api/original-<mode>.stdout`):

| Run | Modes | Result |
|---|---|---|
| 161fx_sysmalloc, `MallocGuardEdges=1 MallocScribble=1` (`raw/guard_161fx`) | all 9 | exit 0, stdout equal ×9 |
| same + `CP_IDS_DISABLE=1` (`raw/guard_161fx_cpoff`) | lifecycle, lifecycle_long, datadir_lifecycle | exit 0, equal ×3 |
| 161asan (`raw/asan_fx`) | all 9 | exit 0, **0 ASan reports**, equal ×9 |
| 161asan + `CP_IDS_DISABLE=1` (`raw/asan_fx_cpoff`) | 3 lifecycle modes | exit 0, 0 reports, equal ×3 |
| 161fx mimalloc link (`raw/mimalloc_161fx`) | all 9 | exit 0, equal ×9 |
| **control** 158fp_sysmalloc + guard (`raw/guard_158fp_control`) | 3 lifecycle modes | exit 139 ×3 |
| **control** 161asan0b (`raw/asan0b`, `asan0b_cpoff`) | lifecycle modes | 1 ASan report each (faults 1/2 above) |

The 9 modes:
- default, features;
- lifecycle;
- lifecycle_disable (re-setup with `CRF_INT_KEYS_DISABLE`, which now really exercises the string path: role ids no longer survive);
- lifecycle_long (41 tokens);
- datadir, datadir_features;
- datadir_lifecycle (4 setups, printing on and then off);
- datadir_bad.

The only stderr difference is the log `__FILE__:__LINE__` in datadir_bad, the same as 158.

Attempt 1 of guard_161fx and mimalloc_161fx was **invalid**: zsh did not word-split the mode list, so one argument was passed and only
`default` ran. It is kept as `raw/*_attempt1_INVALID_zsh_nosplit`; the bash rerun is the result above.

**Item 2:** inventory above.

**Item 3:** full suite (`tools/verify.sh 161fx`, sequential, `raw/verify_161fx/`, 15/15 exit 0, empty stderr):
- golden-v1 45574ee2… and novel 806b8a81… as expected;
- OSM v3, unseen v4–v11, 150 holdout, DAD: IDENTICAL;
- frozen golden-v1 and novel: IDENTICAL;
- fuzz s1/s2 × 2M: IDENTICAL.

The feature-print stream (158's `feat_dump`) on dev, holdout and DAD is byte-identical to the original's (`raw/feat/compare.txt`). Reserved
v12/v13 were not opened. Negative controls (`raw/negative_control/result.txt`): a mutated v1 dump fails cmp and compare_frozen; mutated fuzz
and feature files fail cmp.

**Single thread:** no thread primitives in the diff. `ps -M` on the running 161fx `fuzz_pipeline`: 1 thread in 6/6 samples at 97–100% CPU
(`raw/thread_check.txt`). Attempt 1 found no pid because it was queued for a slot; it is kept.

**Parse path untouched (structural):** per-function normalized disassembly of `dump_outputs`, 161fx vs 158fp (`raw/disasm/`).
`_address_parser_parse`, `_address_parser_features` and `_is_valid_dictionary_phrase` differ only in `__LINE__` immediates of error-log calls
(+4). Every other differing function is setup or teardown code. `feature_array_add_nb` and all CRF scoring and Viterbi functions are identical.

## Known limitations

- The fa_nb fix is a deliberate leak of about 7 KB per teardown, not cache invalidation. Lead decision (2026-10-05, after integration): keep the
  bounded leak, since a stale name id would be an output hazard.
- Same-model re-setup is what the probes test. A *different* model through a second datadir in one process was not tested, because only one
  model is available.
- **The transliterate.c reset is added but not exercised.** Every probe calls `libpostal_teardown()` only once, at exit, so nothing re-derives
  the `ts_*` facts after the reset. Repeated full-library cycles (`libpostal_teardown` → `libpostal_setup`) were not probed.
- Under ASan only the first fault per process is reported (no `-fsanitize-recover`). The faults were found iteratively: fault 1, then
  `CP_IDS_DISABLE` for fault 2, then a full clean pass after the fix.

## Decision

- Items 1–3: **PASS**.
- Item 4: **OPEN** (TIMING_REQUEST.md, 161fx vs 158fp, same session).
- Next: lead timing; on pass, 161fx supersedes 158fp as the candidate.
- Ledger, report and activity log: not updated (read-only for teammates).
