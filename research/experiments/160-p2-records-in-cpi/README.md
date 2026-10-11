# Experiment 160 — P2: compiled records inside cpi (feature path)

Date/time/timezone: created 2026-10-05T17:16:01-0700 (`raw/created.txt`), America/Los_Angeles.
- Teammate p2 worked until 18:05. Teammate p2b took over at 19:16 (`raw/commands.txt`).

Status: **correctness complete for 160b and 160c; timing pending.** The timing request is in `TIMING_REQUEST.md`. The kill rule (`KILL_RULE.md`, pre-registered, not edited) is open on gain and memory.

## Hypothesis and change

**Bottleneck.** In cpi, the feature function emits ≈ 55 keys per address: 702,370 O3i records over 12,863 golden-v1 addresses, most of them as strings. Each key is looked up in two large dictionaries, and its row is then read through `indptr` and a packed-row array. 153 itemizes what this costs: dictionaries 529.5 + vocabulary 115.3 + packed rows 471.7 + `indptr` 54.2 = 1,170.7 MB.
- 157 measured string building + lookups at ≈ 1.0 µs of cpi's 2.1 µs features + lookups (golden-v1, cold).
- 155 measured the record design's scoring block at 2.111 / 1.509 µs (golden-v1 / OSM) against cpi's 3.155 / 2.308.
- 157's gate projected +4–8% (UNVERIFIED).

**Change: one offline file derived from the model only.** `tools/gen_records.c` reads the model (`crf_load` of the original library) and 143's key dumps. It writes `artifacts/records/address_parser_records.dat`:
- **Size and contents.** 1,117.3 MB: 10,025,721 records, 13,557,097 slots, 3,694,560 vocabulary entries and 57 families.
- **Format.** Defined in `sources/rec_format.h`: 153's grouping, f64 values in model row order, and a power-of-two index.
- **Use at setup.** The parser maps the file read-only and touches every page.
- **Structures it replaces:**
  - the state and transition feature tries and dictionaries;
  - both packed weight-row arrays and `indptr`;
  - the vocabulary trie and dictionary.

  `crf_read` seeks past those sections. `transition` weights are still read.

**Stages:**

| Build | Sources | Switches | What changes |
|---|---|---|---|
| 160ctl | `sources/` built **without** `EXP_RECORDS` (= 119-cpi sources, all 15 hashes equal) | cpi's | Nothing. This is the rebuild control. |
| **160b** | `sources/` | cpi's + `EXP_RECORDS` | Key strings are still built. In the scoring pass they are resolved to slot pointers (`rec_lookup_str`) and then added per token in emission order (152 PROOF §6). Vocabulary frequency comes from the record. FTT is re-keyed by vocabulary ordinal. `print_features` has a records path. |
| **160c** | `sources_c/` | cpi's + `EXP_RECORDS` + `EXP_RECORDS_DIRECT` | The emitters (`feature_array_add_nb`/`_nbl`) push (record, slot) **directly**, in the original emission order. No key string is written. The call-site class (unit / pair / phrase-type / own / generic) is cached per call site. A per-address, content-verified record memo means neighbour words (word, prev word, next word, vocabulary frequency of the same token) reuse one record and are never carried across addresses. |
| 160bo, 160co | same + `EXP_RECORDS_ORACLE` | PGO none | Diagnostic oracle dumps only. Never timed. |
| 160bdiag | `sources_diag_capreset/` | cpi's + `EXP_RECORDS` | **Diagnostic only.** It tests the inherited-defect hypothesis below. It is not a candidate. |

Patches: `patches/160b_vs_119cpi.diff`, `patches/160c_vs_160b.diff` and `patches/diag_capreset_vs_160b.diff`. Source hashes are in `raw/source_hashes.txt`.

**Rules 1–3.**
- **Single thread:** the code adds no thread primitives (`raw/thread_primitive_scan.txt`; its 7 matches are comment words such as "compiled").
- **Model-derived only:** tables, families and layout come from the model and key dumps, never from a corpus.
- **No cross-address cache:** the memo generation is bumped before every address, and a memo entry is only returned after its key bytes compare equal.

## Reproduction

**Hardware and toolchain** (`raw/environment.txt`): Apple M4 Pro, 24 GB, macOS 26.5.2 (25F84), Apple clang 17.0.0 (clang-1700.6.3.2). The build is `tools/build_cc.sh` (119's recipe, `E=` retargeted): whole-library PGO + full LTO, mimalloc, tokenizer at -O1.

**Build commands:**
```sh
EXTRA_SW="$(cat raw/switches_160b.txt)" tools/build_cc.sh 160b clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
SRCDIR=sources_c EXTRA_SW="$(cat raw/switches_160c.txt)" tools/build_cc.sh 160c clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
EXTRA_SW="$(cat raw/switches_cpi.txt)" tools/build_cc.sh 160ctl clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
```

**PGO training** is on `profiling/golden-v1/development.txt` only. p2b changed `build_cc.sh` to save the training output to `raw/pgo_train_<name>/`; no compile flag changed. For both 160b and 160c that output has 10,308 of 10,308 lines and exit 0.

**Records generation:** `artifacts/gen/gen_records <model dir> <keys143 dir> <out.dat> <slot_ids.u32>`. Its log is in `raw/gen_records/`, with exit 0 and VERIFY PASS:
- read back from the written file, all 13,557,097 rows are bit-identical (0 mismatched, 0 unmapped);
- all 3,694,560 vocabulary counts are equal.

p2b re-hashed the file and it still matches (`raw/housekeeping/rehash.txt`). `tools/rec_scan.c` (`raw/rec_scan/`) passes:
- all 6,331,161 non-vocabulary records have freq 0, which `rec_vocab_freq` relies on;
- the vocabulary list is consistent;
- the arena walk reaches exactly `nslot`.

**Hashes:**
- **Model:** `raw/model_hashes.txt`.
- **Key dumps:** equal to 143 (`raw/keys/`).
- **Artifacts:**
  - `artifacts/<build>/hashes.txt`;
  - timing set: `raw/timing_artifact_hashes.txt`;
  - probes: `artifacts/probes/hashes.txt`.

### Build history (every attempt kept)

- **Base sources.** Checked against `final/119-cpi/SHA256SUMS` by p2 (`raw/base_sources_check.txt`). p2b's re-check (`raw/housekeeping/base_sources_recheck.txt`) shows 13 files OK, plus `crf.c` and `address_parser.c` changed by `EXP_RECORDS`, as expected. The first re-check ran from the wrong directory; it is kept as `..._attempt1_wrong_cwd`.
- **160ctl.**
  - Src hashes are all equal to 119's `build_hashes.txt`.
  - Binary hashes differ, as expected from a different profile path and build (`raw/housekeeping/160ctl_vs_119_hashes.txt`).
  - Outputs are identical to the original everywhere (`raw/verify_160ctl/summary.txt`).
- **gen_records.** Attempts 1 and 2 failed to build (missing `HAVE_CONFIG_H`, then the data dir); attempt 3 succeeded.
- **160b attempt 1.** Interrupted (`raw/build_160b_attempt1_interrupted`). A 50-address smoke run on it is `raw/b_verbose_on_attempt1_partial`; it was not a verification.
- **160b attempt 2.** Exit 0, but the lead killed a run believed to be its PGO training, so it was labelled partial. Its artifacts, logs and PGO cache are kept with the suffix `_attempt2_partialPGO_do_not_time`.
  - **Finding:** attempt 2's merged `dev.profdata` is **byte-identical** to attempt 3's (100% edge overlap; `raw/housekeeping/pgo_attempt2_vs_3.txt`). Training on the records build takes about 5 s, so attempt 2's profile was in fact complete.
  - Its binaries still differ in hash from attempt 3's. Archive timestamps or LTO temp paths are a likely cause (UNVERIFIED).
  - Attempt 3 is the build of record.
- **160b attempt 3** (`raw/build_160b_attempt3`) and **160c** (`raw/build_160c`): exit 0, with no new compiler warnings against 160b (only line numbers shift).
- **160co.** Attempt 1 (O3i identical on all 4 corpora) is kept as `*_attempt1_no_counters`. Attempt 2 only adds diagnostic counters.

## All measured trials

None. Teammates do not time (brief rule 5); see `TIMING_REQUEST.md`.

**Load-only memory and setup**, recorded as *indicative, under parallel load* (`raw/memory_indicative/`, other teammates' model processes were running). Units are MiB from `/usr/bin/time -l`:

| | wall s (load only) | max RSS | peak footprint |
|---|---:|---:|---:|
| original | 3.18 | 2,182 | 2,491 |
| cpi | 12.77 | 2,002 | 2,401 |
| 160ctl | 12.76 | 2,354 | 2,401 |
| 160b | 3.74 | 1,832 | 1,020 |
| 160c | 3.06 | 2,085 | 1,020 |

**Steady state on a live 2M fuzz run** (`vmmap --summary` + `footprint`, `raw/memory_indicative/live_*.txt`):
- 160b and 160c: footprint 523 MB, plus the mapped record file 1.0 GB resident;
- 160ctl: footprint 1.9 GB.

**Memory accounting caveat (decides how the kill item is read).**
- The record file is mapped `MAP_PRIVATE`, read-only, and prefaulted at setup (`rec_open` touches every 16 KiB page) so the first parse does not pay page faults.
- Its pages are clean and file-backed. macOS phys_footprint ("peak memory footprint", `footprint`) does **not** count them; RSS does.
- Footprint alone therefore under-states 160's memory by up to 1.07 GiB.
- Under memory pressure the kernel can drop these pages and re-read them from the file. That may explain part of the RSS variation (UNVERIFIED): cpi and 160ctl, which map no record file, also differ by 350 MB.
- `TIMING_REQUEST.md` declares the decision metric beforehand: P = peak footprint + the whole file size, with max RSS reported beside it.
- Indicatively, P ≈ 2,086 MiB against the original's 2,491 − 20 MiB. This is UNVERIFIED until it is measured under timing conditions.

**Setup time.** The load-only memory runs incidentally showed 3.1–3.7 s of wall time, against cpi's 12.8 s, with the record file warm in the page cache. That is not a timing measurement and decides nothing (brief rule 5): `TIMING_REQUEST.md` asks for `setup_seconds`. A cold page cache was not measured.

## Correctness and interpretation

All checks used one model process at a time through `research/tools/model_slot.sh`.

**1. O3i per-token hit-id sequence.**
- `tools/run_o3i.sh` on the oracle builds compares full lines **including `-` misses**, which is stronger than the binding condition (misses dropped), against `research/oracles/v1` (read-only).
- 160bo and 160co (attempts 1 and 2) are **identical** on all four corpora, with candidate sha = oracle sha:

  | Corpus | O3i records |
  |---|---:|
  | golden-v1 | 702,370 |
  | novel | 713,995 |
  | osm-v3 | 546,890 |
  | fuzz-s1-200k | 14,030,294 |

- **Negative control** (`raw/o3i_160bo/negative_control.txt`): an id+1 swap at line 400,001 and a miss turned into id 0 both fail `cmp`. `oracle_cmp.py` names address 7337, token 6.
- **160co counters** (golden-v1): own 68,894; unit 407,649; pair 123,008; ptype 102,819; generic 0. The memo returned 600,169 records against 237,208 index lookups. The generic fallback was never taken on any corpus, fuzz included. It is correct by construction (it is stage B's verified `rec_lookup_str`), but untested by these corpora.

**2. Byte identity against the original** (`tools/verify.sh`, a copy of 158's with `E` retargeted; `raw/verify_{160b,160c,160ctl}/summary.txt`). 160b, 160c and 160ctl are all **identical**, with 0 stderr bytes:
- golden-v1 sha `45574ee2…` and novel sha `806b8a81…`;
- osm-v3 and unseen-v4 through v11;
- the 150 holdout (`audit-input.txt`) and dad-international;
- both frozen references (`profiling/golden-v1` and `golden-v2-novel` `baseline-outputs.jsonl`);
- fuzz seeds 1 and 2 × 2,000,000.

The kill rule's "13 non-reserved sets" map onto what exists:
- the 11 non-reserved suite sets (golden-v1, novel, osm-v3, unseen-v4…v11);
- the 150 holdout;
- dad-international.

v12 and v13 are reserved and were not opened.

**Negative control** (`raw/verify_160b/negative_control.txt`): one changed byte makes the OSM `cmp`, the golden-v1 sha and frozen comparison, and the fuzz `cmp` report DIFFERENT.

**3. print_features(true)** (`tools/probes.sh` with r0's `feat_dump.c`, sha equal to r0's copy; `raw/probes/summary.txt`):
- On golden-v1 development (10,308 addresses), `audit-input.txt` and `dad-international.txt`, 160b and 160c are **identical** to the original, with exit 0 and empty stderr.
- The original dumps produced here have the same sha as r0's `158/raw/feat/original-*.txt.gz`.
- 160ctl aborts (exit 134), as cpi does under `EXP_ROW_PACK`; r0's 158 repairs that.
- Under `EXP_RECORDS` the print path never reaches the structures r0 listed:
  - `crf_tagger_score` returns into `crf_tagger_score_records` before the dictionary paths, so there is no `fd_lookup` on `state_dict` or `trans_dict`, and no `indptr`, packed rows or `crf_feature_ids_reserve`;
  - the dictionaries are NULL, so `crf_ik_active` stays 0, `crf_role_ids` stays NULL and cp_ids is off;
  - the feature function with `crf_features_accumulate == 0` only builds strings, which go through `rec_lookup_str`.

**4. API probes** (`api_probe2`, r0's, in all 9 modes against the original):
- Under mimalloc, 160b and 160c match the original's stdout and stderr in every mode, `datadir*` and `lifecycle_long` included. As §5 shows, the re-setup modes match despite out-of-bounds writes; those passes are luck.
- The one exception is `datadir_bad` stderr, where only the source path and line number in the log message differ (`raw/probes/summary.txt`).
- A missing record file makes setup fail cleanly, with exit 3 and "Error loading address parser module". This was seen by accident when lldb could not open the Desktop path (`raw/probes/lldb/*attempt1_tcc_denied.txt`).

**5. Inherited defect: teardown and re-setup.**
- **Symptom.** Under system malloc with `MallocGuardEdges=1 MallocScribble=1` (`raw/probes/summary.txt`, guard section), the original passes `lifecycle`, `lifecycle_long` and `datadir_lifecycle`. The cpi-derived builds crash:

  | Build | lifecycle | lifecycle_long | datadir_lifecycle |
  |---|---|---|---|
  | 160ctl | 139 | 139 | 139 |
  | 160b | passes | 139 | passes |
  | 160c | passes | 139 | 139 |

  The crash comes on the first parse after re-setup. Backtraces show heap corruption detected later, inside `realloc` from `feature_array_add_nb` (`raw/probes/lldb/`).
- **Mechanism (source).** `crf_lean_cap_items` (`EXP_LEAN_CONTEXT`, `crf.c`) is a static that `crf_destroy` does not reset. After re-setup, the fresh context has the default 10-item buffers. Any address with 10 < T ≤ the stale capacity (32 or 64) is therefore written past them.
- **Test.** 160bdiag is 160b plus `crf_lean_cap_items = 0` in `crf_destroy`; its PGO is "none".
  - It passes all three guard modes, with stdout equal to the original.
  - The unpatched no-PGO control, 160bo, crashes in `lifecycle_long` and `datadir_lifecycle` (`raw/probes/diag_capreset/summary.txt`).
- **Relation to 158/161** (lead message, 2026-10-05). The lead verified that cpi itself crashes on re-setup from a stale `cp_pool` in `cp_dfs`, which r0 is fixing in 161. 160b and 160c never build `cp_pool`, because `crf_state_dict_of` is NULL under `EXP_RECORDS`. So their crash is the **second** inherited stale static, `crf_lean_cap_items`, shown by 160bdiag. Every failure is shared with 160ctl, so none is a 160 regression and none is fixed here (lead instruction). A fix in 161 that only addresses `cp_pool` would **not** cover a records build; it also needs the `crf_lean_cap_items` reset.
- **Scope.** The defect comes from cpi, not from 160. It only matters for teardown followed by setup in one process; `crf_ik_active` and `crf_role_ids` are inactive under `EXP_RECORDS`. 160b and 160c are deliberately **not** patched, so that timing isolates the records change. The repair belongs to R0 (158).

**6. Threads.**
- `ps -M` on the running `fuzz_pipeline` PID (found with `pgrep -f`, not the `model_slot` wrapper) shows 1 thread each for 160b and 160c (`raw/thread_check.txt`).
- The scan for thread primitives over the added lines is clean.

**Development vs validation.** PGO uses golden-v1 development only. Every correctness check above uses the full sets. No variant was chosen on correctness data.

## Known limitations and remaining uncertainty

- **The record file path is compiled in.** It is this experiment's artifact, overridable with `LIBPOSTAL_RECORDS_FILE`. A packaged build would ship the file next to the model.
- **Model check is cheap.** The file is checked against the model by the sizes of `address_parser_crf.dat` and `address_parser_vocab.trie` only, not by a hash.
- **The record file adds 1.1 GB on disk.** Shipping it through GitHub would need LFS or a release asset (GitHub's per-file limit is 100 MB; UNVERIFIED for this repository's setup).
- **Offline generation breaks the memory rule if it runs on the target.** `gen_records` took 12.4 s and peaked at a 3.38 GB footprint (3.53 GB max RSS; `raw/gen_records/stdout.txt`), above the original's ≈ 2.49 GB. Generating on the professor's Mac would break "memory ≤ original" for that step; shipping the file avoids it. That is a packaging decision for the lead.
- **Regenerating needs inputs that may disappear.** The generator reads 143's key dumps from the lead's session scratchpad, which is transient. Regeneration needs 143's `keydump` re-run; the expected hashes are in `raw/keys/`.
- **Generic fallback untested.** The stage C generic fallback (a name or phrase type containing `|`) was never exercised by the corpora (counter 0).
- **The memory and setup numbers are indicative** (parallel load, warm page cache). Nothing decides the memory kill item until the lead's run.
- **Debug hard link removed.** A hard link `/tmp/libpostal-160/rec_link.dat` (same inode) was used only for lldb and has been removed.
- **Command history before 19:16.** `raw/commands.txt` was a single line when p2b took over. p2's history exists only as the per-command directories under `raw/` (cmd, start, end, exit). Nothing earlier was reconstructed.

## Decision

**Pending timing.**
- **Output difference: none.** That kill item is decided as passed for 160b and 160c.
- **Peak memory and gain ≥ +3% against cpi:** open. They are decided by `TIMING_REQUEST.md`.
- **Next action:** the lead runs `tools/run_timing.sh`. If either variant passes, it goes to the milestone protocol (novel, unseen-v12 once).
- **Ledger, report and activity log:** not updated. Those are the lead's files (brief: read-only for teammates).
