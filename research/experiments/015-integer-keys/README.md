# Experiment 015 — Integer-keyed feature lookup (symbol interning), on top of 011 `h`

Date/time/timezone: started 2026-09-30 14:28 PDT
Status: **small verified increment** — `k1` is +2.6% (golden-v1) / +2.1% (novel) faster than the rebuilt control `h0` by median, byte-identical, single thread, lower memory. Pairs (`k2`) and the vocabulary memo alone (`m`) add nothing measurable. A 20-round paired session is recommended before folding it into the final candidate.

## Hypothesis and change

Measured bottleneck and why this change should help: experiment 014 showed the parse is instruction-bound, not memory-bound (memory floor
0.25-1.16 µs of 13.22 µs). Building feature key strings, scanning them for `|`, hashing them and byte-comparing them against a second cache line
(the arena key) is a large instruction cost (profile of `h`: `feature_array_add`, `fd_split`/`memchr`, `memcmp`, `memmove`). 014 Q6: 76% of the
562,743 development lookups have value parts that are all vocabulary words; word-role features are half of all lookups.
Chen & Manning (2014) found feature string concatenation and lookup to be >95% of runtime in parsers of this design.

Exact implementation change (switch `EXP_INT_KEYS`, sub-switch `EXP_INT_KEYS_PAIRS`):
1. One canonicalization function `canon(s)`: the vocabulary dictionary's slot index of `s` if `s` is non-empty, contains no `|`, and is a key of
   the vocabulary dictionary; otherwise NONE. `UNKNOWN`/`UNKNOWN_NUMERIC` are therefore not special: they are internable only if they are vocabulary keys.
2. At load (after the vocabulary dictionary exists), each feature dictionary (state, transition) is split: a key whose name is a routed name and whose
   value splits on `|` into exactly the name's arity of parts, each with canon != NONE, moves to an integer table keyed by
   (name id, canon(part1), canon(part2)); every other key is re-inserted into a new string dictionary. Checks: moved + kept = old key count;
   a duplicate integer key or an id that does not fit the packing refuses the split (keeps `h`'s dictionaries).
   Routed names: state `word`, `prev word`, `next word`, `first word` (unary) and, with `EXP_INT_KEYS_PAIRS`, `prev word+word`, `word+next word`;
   transition `word`, `prev word`.
3. One lookup function over both tables is used by every reader of the feature dictionaries (`crf_get_feature_id`, `crf_get_state_trans_feature_id`,
   the batched path and the direct path): a string key whose name is routed and whose parts all have canon != NONE is looked up in the integer table,
   anything else in the string table. So a string query returns exactly what the unsplit dictionary returned.
4. Emission (only when `crf_features_accumulate` is set and the integer tables exist): at each routed feature, canon() is computed from the final
   value strings at that moment; if all parts are internable, an empty placeholder token is added to the feature array and the integer key is appended
   to a side list; otherwise the string is built as before. The lookup pass reads placeholders from the side list in order, so accumulation order is unchanged.
5. Verification switch `EXP_INT_KEYS_VERIFY`: keeps the untouched full string dictionary and checks every lookup (placeholder and string) against it,
   aborting on the first mismatch.

Base: 012 candidate sources (= 011 `h` plus switches that are off); copied to `candidate/` (start hashes `raw/start_sources.sha256`).
Variants: `h0` control rebuilt from 015 sources without the switch; `k1` unary routed names; `k2` unary + pairs; `kv` = `k2` + verify (correctness only).
Incremental comparison base: `h0`; cumulative base: preserved original library (000-baseline).

Prediction: none. 012's inline hash/compare was slower than expected, so the string cost may be smaller than its profile share. If the effect is
under ~2%, the 20-round paired protocol is used.

## Reproduction

Hardware/OS/compiler: Apple M4 Pro, 24 GB, macOS Darwin 25.5.0, Apple clang 17.0.0. Build: `tools/build.sh` (log `raw/build1.log`; `kv` rebuilt with an exit
report, `raw/build2_kv.log`); per-variant flags and hashes in `artifacts/<v>/hashes.txt`. The new switches are passed to both `crf.c` and
`address_parser.c` (the dictionary struct layout depends on them). Baseline harness rebuilt from the current `research/tools/bench_ext.c` and the
preserved `000-baseline/artifacts/libpostal.a` (`artifacts/baseline_bench_hashes.txt`); `h` is 011's preserved binary.
Timing: `tools/run_timing.sh` = `research/tools/run_unattended.sh raw/session1 5 19 baseline h h0 m k1 k2` (AC gate, caffeinate, telemetry,
rotating order), launched 2026-09-30 14:42:27 PDT after the fuzz finished, 60 runs, `DONE`, no `ABORT`, no gate waits. Analysis:
`analyze_ext.py` -> `raw/session1-summary.md`, `analyze_quiet.py` -> `raw/session1-quiet.md`. (The background wrapper reported exit 1 only because
its final `cat raw/session1/ABORT` found no such file; the session itself exited 0.)

## All measured trials (5 processes x 18 pooled warm passes per variant and dataset; full per-run logs in `raw/session1/`)

| dataset | variant | median µs | P10 µs | CPU-time median µs | cold pass1 µs | speedup vs baseline (median) | vs `h0` |
|---|---|---:|---:|---:|---:|---:|---:|
| golden-v1 all | baseline | 35.50 | 34.75 | 35.47 | 36.47 | 1.000x | |
| golden-v1 all | `h` (011) | 13.30 | 13.16 | 13.27 | 13.64 | 2.669x | |
| golden-v1 all | `h0` control | 13.26 | 13.11 | 13.24 | 13.47 | 2.677x | 1.000x |
| golden-v1 all | `m` memo | 13.23 | 13.07 | 13.22 | 13.39 | 2.683x | 1.002x |
| golden-v1 all | **`k1`** | **12.93** | 12.78 | 12.91 | 13.09 | **2.745x** | **1.026x** |
| golden-v1 all | `k2` | 12.93 | 12.82 | 12.92 | 13.31 | 2.746x | 1.026x |
| novel | baseline | 35.61 | 35.16 | 35.58 | 37.28 | 1.000x | |
| novel | `h` (011) | 13.21 | 13.09 | 13.21 | 13.46 | 2.695x | |
| novel | `h0` control | 13.26 | 13.15 | 13.25 | 13.41 | 2.686x | 1.000x |
| novel | `m` memo | 13.24 | 13.13 | 13.23 | 13.60 | 2.690x | 1.002x |
| novel | **`k1`** | **12.99** | 12.76 | 12.98 | 13.26 | **2.741x** | **1.021x** |
| novel | `k2` | 13.09 | 12.91 | 13.07 | 13.40 | 2.720x | 1.013x |

All estimators (median, P10, CPU time, cold first pass) agree on the direction. Every pass was flagged as preempted (shared laptop), as in 012.
Per-country (>= 100 addresses): `k1` minimum 2.233x (lt) / median 2.681x on golden-v1; no country below 1.05x.
Setup time: `h0` 6.5 s, `k1` 9.2 s (+2.7 s), `k2` 12.0 s (+5.4 s) — the load-time split.
Peak footprint (`/usr/bin/time -l`, 10 processes each; median): baseline 2,613,758,208; `h0` 2,557,102,240; `k1` **2,538,457,272** (-18.6 MB vs `h0`,
-75.3 MB vs baseline); `k2` 2,544,732,344.
Steady-state footprint (`footprint -p` during warm passes, `raw/steady_*.footprint`): baseline 2,493 MB, `h0` 2,439 MB, `k1` **2,421 MB**. Dictionary sizes (`CRF_DICT_VERBOSE`): `k2` state 307.3 -> 204.9 + 67.6 MB int, transition 47.3 -> 17.5 + 21.7 MB.

### Floor subset (added 2026-09-30 15:16-15:19 PDT, AC gate before every run, no waits)

`tools/run_floor.sh`: baseline, `h0`, `k1` on 013's `floor_non_ascii_only` (5,404 real addresses the ASCII fast path cannot help), 5 processes each,
rotating order, 15 timed passes after one untimed warm-up pass on the novel set (013's per-run protocol). Analysis `tools/analyze_floor.py` -> `raw/floor-summary.md`.

| variant | warm median µs | P10 µs | per-process medians | cold pass1 µs | speedup vs baseline | vs `h0` |
|---|---:|---:|---|---:|---:|---:|
| baseline | 33.41 | 32.91 | 33.08-33.57 | 34.15 | 1.000x | |
| `h0` | 14.51 | 14.37 | 14.44-14.61 | 14.96 | 2.302x | 1.000x |
| `k1` | 14.17 | 14.01 | 14.02-14.23 | 14.50 | **2.358x** | 1.024x |

Measured floor of the new best build: **2.36x** (013 measured 2.33x for `h` in a different session; the two sessions are not mixed). The per-process
ranges of `h0` and `k1` do not overlap. `k1` peak footprint on this run 2,537,998,544 bytes.

## Correctness and interpretation

- Byte-identical: `h0`, `m`, `k1`, `k2`, `kv` equal the baseline library's dump on golden-v1/all (sha256 `45574ee2...`) and the novel set (`806b8a81...`) (`raw/outputs_*`).
- Frozen references (`tools/compare_frozen.py`, keyed by sha256(address)): `k1` and `h0` identical to `profiling/golden-v1/baseline-outputs.jsonl` and
  `profiling/golden-v2-novel/baseline-outputs.jsonl` (12,863/12,863 each; `raw/frozen_compare.txt`). Negative control: one changed byte in a copy of `k1`'s
  dump is reported DIFFERENT, exit 1 (`raw/frozen_negative_control.txt`, `raw/negative_control_k1_v1.jsonl`).
- Lookup-level differential (`kv`, keeps the unsplit dictionaries and checks every lookup, aborts on the first mismatch): golden-v1 702,370, novel 713,995,
  validation 139,627, development 562,743 lookups (= 014's count), 0 mismatches (`raw/dump_kv2_*.stderr`).
- End-to-end fuzz (`research/tools/fuzz_pipeline`, seeds 1 and 2 x 2,000,000 over golden-v1 + novel; the corpus/seed choice was inferred by reproducing the first
  1,000 lines of 011's saved baseline output): `kv` and `k2` per-input hashes identical to 011's baseline output for both seeds; `kv` checked 140,120,259 and
  140,223,241 lookups, 0 mismatches (`raw/fuzz/summary.txt`); `k1` (the chosen candidate) fuzzed separately afterwards, both seeds identical.
- Single thread: `ps -M` on a running `k1` shows 1 thread at 5/10/15 s (`raw/threadcheck_k1.txt`); no thread primitives added.
- Interpretation: the gain is real but small (~2-2.5%), far below what the instruction-bound argument of 014 might suggest. The vocabulary memo alone (`m`) gives
  nothing, so repeated vocabulary lookups were already cheap; pairs (`k2`) add nothing on golden-v1 and lose ~0.8% on novel versus `k1`, plausibly because pair
  features need an extra canonicalization when a part is a phrase and pair keys often miss (UNVERIFIED: no profile of `k1`/`k2` taken). As with 012, removing
  string handling saves less than its profile share: much of the per-feature cost is elsewhere (building the feature list, weight rows, the other 50% of lookups).
- Limitations: one session; effect near the ~2% level where the protocol recommends the 20-round paired comparison; shared laptop (all passes preempted).

## Decision

Small verified increment: **`k1` (EXP_VOCAB_MEMO + EXP_INT_KEYS, no pairs)** becomes the best candidate, 2.745x (golden-v1) / 2.741x (novel) vs the original
baseline, byte-identical, single thread, lower memory, +2.7 s setup. `k2` and `m` are not carried forward: `k1` is chosen over `k2` for its 2.7 s shorter setup and ~6 MB smaller footprint (they tie on golden-v1 time); the
novel-set timing is not used for the choice.
Next action (not yet run): a 20-round paired `h0` vs `k1` session to pin the increment; then profile `k1` before the next experiment.
