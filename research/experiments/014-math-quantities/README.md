# Experiment 014 — mathematical quantities Q1-Q7 of ALGORITHM_RESEARCH.md section 6 (measurement only)

Date/time/timezone: 2026-09-30, 13:24-14:20 -0700
Status: complete for Q1-Q7. Q8 (optional) not attempted. Q2 was first refused by the AC gate (battery; `raw/q2_gate_attempt1_battery.txt`,
`raw/q2_gate_attempt2_waiting.txt`) and ran after the laptop was plugged in.

## Hypothesis and change

Nothing in libpostal was changed and nothing here is a speed claim. The experiment computes the quantities listed in section 6.8 of
`research/analysis/ALGORITHM_RESEARCH.md` (moved there 2026-09-30), which decide whether the algorithmic items 3.1 (per-word precomputation) and 3.2 (integer-keyed lookup) are worth building.
Base: the preserved baseline library (`000-baseline/artifacts/libpostal.a`); build h's data structures are rebuilt from the loaded model with h's own
`feature_dict.h` (sha256 `9c334a86…`, identical to `011-normalize-fastpaths/candidate/feature_dict.h`) using the same calls as h
(`fd_build` for the state and transition tries, `fd_build_ex(vocab, false, true)` for the vocabulary).

## Reproduction

Hardware/OS/compiler: Apple M4 Pro, 24 GB (`hw.memsize` 25769803776), 16 KiB pages, **128-byte cache lines** (`hw.cachelinesize`), L1d 128 KiB and
L2 16 MiB per P-cluster (`hw.perflevel0.*`); macOS Darwin 25.5.0; Apple clang 17.0.0 (clang-1700.6.3.2).
Corpus: `profiling/golden-v1/development.txt` only (10,308 addresses, sha256 `252960e6…`). Model files (sha256 match `profiling/manifest.json`):
`artifacts/input_hashes.txt`. Tool/binary hashes: `artifacts/hashes.txt`.
Every command ran through `tools/run.sh NAME CMD…`, which stores `raw/NAME.{cmd,stdout,stderr,meta}` (meta = start, end, exit code).

```sh
E=research/experiments/014-math-quantities
$E/tools/run.sh q1_amdahl python3 $E/tools/q1_amdahl.py            # Q1 (arithmetic; asserts it reproduces the 6.4 table)
$E/tools/run.sh build4 $E/tools/build.sh                           # trace + chase against the preserved baseline library
$E/tools/run.sh trace_dev $E/artifacts/trace trace profiling/golden-v1/development.txt $E/data
$E/tools/run.sh vocab $E/artifacts/trace vocab $E/data
$E/tools/run.sh model $E/artifacts/trace model
$E/tools/run.sh an_q3 python3 $E/tools/analyze.py q3   # likewise q4, q5, q6
$E/tools/run_q2.sh                                     # Q2: env gate (waits for AC), caffeinate, telemetry, 2.5 GiB + 8 MiB chase
$E/tools/run.sh an_q2 python3 $E/tools/q2_analyze.py   # then rerun an_q3 for the T_mem_min floor
```

Build log history: `build1` failed (`LIBPOSTAL_DATA_DIR` undefined; the library's `ADDRESS_PARSER_IS_IGNORABLE` macro refers to a variable literally named
`token`), `build2` failed (commas inside `cstring_array_foreach` macro arguments), `build3` succeeded (trace binary `72ec232a…`, used for `trace_dev`,
`vocab`, `model`), `build4` added `chase` and rebuilt trace from the same source as `6e4fd692…`; `build5` fixed the chase layout; `build6` extended it to 32 chains (the knee rule was also changed from "<5% gain per chain", which fires under linear scaling, to "M(k)/k < 0.9"). All three trace modes were rerun with the rebuilt
binary (`*_rerun`) and every output was byte-identical (`raw/rerun_check.txt`). The first `an_q4/5/6` invocations ran before `analyze.py` existed
(file-not-found); their raw logs were overwritten by the real runs.

## Results

Each result is "computed" from the stated inputs; none is a speedup. Full tables: `data/q1_amdahl.md`, `data/q3.md` … `data/q7.md`.

**Q1 Amdahl ceilings** (`data/q1_amdahl.md`). Reaching 3.5× needs 3.25 µs (24.6%) of build h's 13.22 µs removed on the full set and 4.91 µs (33.5%)
of 14.67 µs on the floor set; 5× needs 47.2% / 53.4%. The largest single stage is context_fill (24%, 3.17 µs): even deleting all of it falls just short
(34.88 / (13.22 - 3.17) = 3.47×) on the full set. Per-item ceilings use the narrowest measured share the audit quotes (e.g. P1 12.6% = 1.67 µs, F5 7.3% = 0.97 µs); summed item
ceilings exceed their stage shares several times over because items overlap, so the stage share is the binding bound. The floor-set column applies full-set
profile shares to 14.67 µs (ASSUMPTION: no floor-set profile exists).

**Q2 DRAM latency λ and parallelism M** (`data/q2.md`, `data/q2_chase.json`). Tool `tools/chase.c`: one Sattolo cycle over 2.5 GiB of 128-B nodes
(20,971,520 nodes, 16 KiB-aligned), an index-stored and a pointer-stored variant (the array is rewritten per pass so each holds only its own field),
k interleaved chains starting evenly spaced on the cycle, 20 M loads per point, 5 trials alternating variant and k order, env gate + caffeinate + telemetry.
Final session (k = 1..32, `build6`): **λ = 127.2 ns** (idx) / 126.6 ns (ptr) per dependent load including TLB misses on 16 KiB pages; L2-resident
8 MiB control 10.0 / 9.3 ns. Throughput scales almost linearly: M(16) = 15.8, M(29) = 27.4 (idx), M(32) = 29.7 (ptr, per-chain efficiency 0.93).
**No saturation up to 32 chains**, so M_max = 29.7 is a lower bound on one core's memory-level parallelism. The idx drop at k >= 30 (M 24.0) is a
code-generation artifact: at k = 30 the compiler moves the chain state into NEON lanes (42 `ld1.s`/`mov.s` lane operations vs 0 at k = 29;
`raw/q2_idx30_disasm.txt`), while ptr stays scalar. idx and ptr give the same λ and scaling, so no data-dependent prefetcher effect is visible.
Earlier session (k = 1..16, `build5`, kept as `raw/*_v2k16*`, `data/q2_k16.md`): λ = 135.4 / 126.2 ns, M(16) = 16.2, not saturated either; load average
was 6-9 during both sessions (shared laptop; `raw/q2_env*.txt`), and the two idx λ values differ by 6%.

**Q3 cache lines per address** (`data/q3.md`; build h's layout; state + transition lookups only). Per scored address (10,305; 3 bypass the CRF):
54.6 lookups, 452 line touches, **270 distinct 128-B lines** (median 264; slots 55, arena keys 44, indptr 23.5, indices 58, weight data 89.5).
Distinct lines over the whole split: 592,742 (57.5 per address first-touch). An exact LRU model over this lookup stream alone misses 76.4 lines per address at
16 MiB (181.6 at 128 KiB, 57.8 at 64 MiB); real caches, shared with the tries, vocabulary and normalization data excluded here, would miss more.
**T_mem_min = N·λ/M_max** with λ = 127.2 ns, M_max = 29.7: **1.16 µs per address (8.7% of 13.22 µs) if every address starts cold**
(N = 270 distinct lines); 0.33 µs (2.5%) with N = 16 MiB-LRU misses of this stream; 0.25 µs (1.9%) at 64 MiB. Without any overlap (M = 1) the same 270
lines would cost 34.3 µs, so the lookups must already overlap heavily. Because M_max is a lower bound, these floors are upper estimates of the floor.

**Q4 vocabulary Zipf** (`data/q4.md`). 3,694,560 words, counts sum 3.49 × 10⁹, 1,240,558 words above the rare threshold 50. Coverage C(N):
2^10 0.691, 2^14 0.854, 2^16 0.908, 2^18 0.950, 2^20 0.985. Dense M(N) = N·160 B; the largest N under 40 MB is 250,000 (C = 0.949), under 40 MiB
262,144 (C = 0.950). Zipf fit: s = 1.17-1.19 (R² ≥ 0.998) for ranks up to ~10⁶ / count > 50; s = 1.50 (R² 0.969) over all ranks (the tail steepens).

**Q5 empirical coverage** (`data/q5.md`). 60,593 scored dev tokens; 27,130 plain (bias emitted). Share of plain tokens whose word is in the top N:
0.688 (2^10), 0.825 (2^16), 0.883 (250,000), 0.945 (2^20); the model's C(N) over-predicts by 0.3-8.3 points. **But only 9,739 plain tokens (35.9%)
have `bias` as the first state feature**; the other 17,391 all start with a postcode feature (`raw/q5_before_bias.txt`), so for them the U block is not a
prefix (Lemma 2 fails as stated, Q7 2.6). Restricted to bias-first tokens with count > 50 and word in the top 250,000: 6,730 = 0.248 of plain tokens
(0.185 also without a prefix/suffix feature).

**Q6 integer-key reach** (`data/q6.md`). R over all 562,743 lookups: 0.762 (literal Lemma 5 predicate), 0.768 if `UNKNOWN`/`UNKNOWN_NUMERIC` are
interned (they are not vocabulary words). Word-role features (word, prev/next word and their pairs, first word, sub_word, both transition features) are
57.0% of lookups with R 0.877, i.e. 0.500 of all lookups; phrase-valued features reach D only when the phrase string happens to be a vocabulary word.
Hits only: 0.776. Model keys in D: 5,461,155 of 11,470,623 state keys, 1,424,247 of 2,086,474 transition keys. Separator check: **58 vocabulary words
contain `|` (12 end with it)**; 0 dev tokens do. Constant no-value keys (`bias`, `postcode have context`): 6.1% of lookups.

**Q7 proof obligations** (`data/q7.md`), 27 items: 12 hold, 10 conditional, 2 fail as written, 1 erratum, 2 open. Fail as written: Lemma 2's "U block
is a prefix" (postcode features precede `bias` for 64% of plain dev tokens) and Lemma 3's pair set for one-byte tokens (a one-byte token can begin a
multi-word key; needs a first-byte set). Also: memoized `word_or_phrase_at_index` results must be copied (shared scratch buffers); the address-dictionary
prefilter must be built below the language-prefix node; the prefix/suffix prefilter (P2) remains unproven.

## Correctness and interpretation

Checks: every dictionary answer (state, transition, vocabulary; 562,743 feature lookups + 60,593 vocabulary lookups) equalled the baseline trie:
0 mismatches. The regenerated feature dump is byte-identical to `006-feature-dict/raw/dev_features.tsv` (sha256 `636ee230…`), which documents that
file's provenance (its generator was not preserved). Negative control: the first attempt was invalid (the `sed` pattern matched nothing, so nothing
changed — kept in `raw/compare_006.txt`); the corrected one changed line 1 and `cmp` failed as expected. Single thread: all tools are single-threaded;
no parse-path code was changed.

Errata found in `ALGORITHM_RESEARCH.md` (recorded here and as a dated note there; section 6 text not rewritten): (1) the feature separator is `|`, not a
space (3.2); (2) cache lines are 128 B on this machine; (3) the U block is not a prefix of F_t for postcode-phrase tokens (6.2 corollary); (4) B(K) in
Lemma 3 is insufficient for one-byte tokens. Library observations (pre-existing, not fixed): `ADDRESS_PARSER_IS_IGNORABLE` uses the caller's variable
`token` instead of its argument; `char_array_add_vjoined` reads `arg[-1]` for an empty non-last piece.

Limitations: Q3 counts only the feature-lookup path, with memcmp modelled as reading up to the first differing byte, and the LRU model ignores other data
competing for the caches; Q5/Q6 are development-split only; Q1 shares come from one 15 s profile of h; stage shares overlap.

## Decision

Complete for Q1-Q7. Not an optimization: nothing accepted or rejected.
Ledger and activity log updated; professor report unchanged (no verified speed result changed).
