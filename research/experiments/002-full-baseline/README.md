# Experiment 002 — Complete baseline (measurement only)

Date: 2026-09-29 PDT (throughput trials 12:01-12:05). Status: **complete (items 1-8 done; item 8 = documented limitation)**
Purpose: everything 003+ needs to compare a candidate fairly. No libpostal source, flag, LTO or -O3 change here
(those are changes, not baseline). Baseline binary/libs: `../000-baseline/artifacts` (hashes in `raw/hashes.txt`).
Hash of every harness source is recorded before each run in `raw/hashes.txt`.

## Done-list (written before running; stop when all are done)

1. Throughput per split (development, validation, all), 5 trials each, >=10 s each, one session, split order rotated per trial;
   each run under `/usr/bin/time -l` (peak RSS and peak memory footprint); `pmset` state and inter-trial gaps recorded.
2. Noise band: spread of throughput and of peak RSS/footprint across those trials.
3. Steady-state memory: `footprint -p` / `vmmap --summary` on the running preserved binary after READY, several times, in
   separate runs from the timing trials. Harness unmodified.
4. Stage breakdown in the ORIGINAL lookup-then-accumulate order (lookup+accumulate as one block per token, timers on),
   full corpus and per split; batched split kept only as a labeled approximation; label bug (`free temporaries`) fixed.
5. Pin the CPU Counters column meaning with a calibration binary: pointer-chase, ALU loop, random branches.
   If 001's inferred labels are wrong, append a correction to 001 (no silent edit).
6. Sub-attribution of feature generation and context_fill from a fresh sampling profile of the preserved binary.
7. Lookup detail: hit vs miss ns and trie-step distributions; stage costs split ASCII vs non-ASCII inputs.
8. L1D/TLB miss counts: attempt via the xctrace CLI once; if not selectable, record as limitation.

## Results

Machine state: Apple M4 Pro, 24 GiB, macOS 26.5.2, AC power (charging), `powermode 0`, no thermal warnings (`raw/throughput/pmset-*.txt`).
**Swap was 39.97 of 40 GB used and free RAM ~2 GB during the whole session** (see memory finding below).
Nothing else was benchmarked concurrently; profilers/`footprint` ran only in separate runs.

### 1-2. Throughput per split and noise band (preserved `parser_profile`, 5 trials x 3 splits, 10 s each, rotated order, 5 s gaps)

| Split | Trial | Addresses/s | µs/addr | wall s | CPU s | harness peak RSS | time -l max RSS | time -l peak footprint | run start (PDT) | exit |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|
| development | 1 | 27,340.25 | 36.58 | 10.180 | 10.139 | 1,847,951,360 | 1,847,951,360 | 2,612,529,360 | 12:01:12 | 0 |
| validation | 1 | 28,381.85 | 35.23 | 10.083 | 10.066 | 2,415,247,360 | 2,415,247,360 | 2,464,106,608 | 12:01:31 | 0 |
| all | 1 | 27,273.03 | 36.67 | 10.376 | 10.334 | 2,477,441,024 | 2,477,441,024 | 2,611,267,816 | 12:01:49 | 0 |
| validation | 2 | 27,953.30 | 35.77 | 10.054 | 10.034 | 2,371,633,152 | 2,371,633,152 | 2,372,241,448 | 12:02:08 | 0 |
| all | 2 | 28,069.74 | 35.63 | 10.082 | 10.049 | 2,311,077,888 | 2,311,077,888 | 2,311,653,368 | 12:02:27 | 0 |
| development | 2 | 28,033.19 | 35.67 | 10.296 | 10.282 | 2,611,904,512 | 2,611,904,512 | 2,611,775,696 | 12:02:45 | 0 |
| all | 3 | 28,606.09 | 34.96 | 10.342 | 10.335 | 2,470,166,528 | 2,470,166,528 | 2,470,824,048 | 12:03:04 | 0 |
| development | 3 | 29,044.42 | 34.43 | 10.292 | 10.290 | 2,290,352,128 | 2,290,352,128 | 2,290,911,200 | 12:03:23 | 0 |
| validation | 3 | 28,431.24 | 35.17 | 10.065 | 10.057 | 2,585,378,816 | 2,585,378,816 | 2,586,085,560 | 12:03:42 | 0 |
| development | 4 | 29,230.56 | 34.21 | 10.227 | 10.218 | 2,613,428,224 | 2,613,428,224 | 2,612,660,456 | 12:04:00 | 0 |
| validation | 4 | 28,836.26 | 34.68 | 10.012 | 9.990 | 2,430,451,712 | 2,430,451,712 | 2,431,092,824 | 12:04:18 | 0 |
| all | 4 | 28,210.20 | 35.45 | 10.031 | 10.007 | 2,255,978,496 | 2,255,978,496 | 2,613,299,456 | 12:04:37 | 0 |
| validation | 5 | 28,736.60 | 34.80 | 10.047 | 10.029 | 2,403,549,184 | 2,403,549,184 | 2,404,173,888 | 12:04:56 | 0 |
| all | 5 | 29,390.85 | 34.02 | 10.066 | 10.056 | 2,603,171,840 | 2,603,171,840 | 2,603,911,400 | 12:05:14 | 0 |
| development | 5 | 29,123.57 | 34.34 | 10.264 | 10.242 | 2,613,329,920 | 2,613,329,920 | 2,612,562,152 | 12:05:32 | 0 |

| Split | n | median addr/s | min | max | spread (max-min)/median | CV | median µs/addr | peak RSS min | peak RSS max | RSS range (bytes) | footprint min | footprint max |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| development | 5 | 29,044.42 | 27,340.25 | 29,230.56 | 6.5% | 2.61% | 34.43 | 1,847,951,360 | 2,613,428,224 | 765,476,864 | 2,290,911,200 | 2,612,660,456 |
| validation | 5 | 28,431.24 | 27,953.30 | 28,836.26 | 3.1% | 1.09% | 35.17 | 2,371,633,152 | 2,585,378,816 | 213,745,664 | 2,372,241,448 | 2,586,085,560 |
| all | 5 | 28,210.20 | 27,273.03 | 29,390.85 | 7.5% | 2.45% | 35.45 | 2,255,978,496 | 2,603,171,840 | 347,193,344 | 2,311,653,368 | 2,613,299,456 |

inter-run gaps (s): min 5.0 median 5.0 max 5.0; session 12:01:12 - 12:05:46
trial-order drift (throughput / split median):
  development t1:0.941 t2:0.965 t3:1.000 t4:1.006 t5:1.003
  validation t1:0.998 t2:0.983 t3:1.000 t4:1.014 t5:1.011
  all t1:0.967 t2:0.995 t3:1.014 t4:1.000 t5:1.042

Reading: throughput noise is 3-7.5% (max-min)/median per split, CV 1-2.6%. Early trials run 3-6% slow
(trial 1 dev 0.941, all 0.967 of median) so the first trial of a session should be discarded or preceded by a burn-in.
The `all` median here (28,210) is within 1.4% of 000's 27,821, i.e. same machine state within noise. **Consequence for 003:**
a candidate needs >= 5 alternating trials per side; differences under ~5% are inconclusive, and the ~12.7% harness effect from 001
is only ~2x the noise band.

### 3. Steady-state memory (separate runs, `footprint -p` and `vmmap --summary`; `raw/memory-summary.txt`)
- Steady-state footprint after READY is **flat at 2,172 MB for 30 s** (MALLOC_LARGE 2,110 MB + MALLOC_SMALL 59 MB), identical for
  `all` and `development`. No growth: no leak in the loop.
- Load phase peaks at 2,282 MB (all) / 2,316 MB (development) at t=2 s; `time -l` peak footprint 2.29-2.61 GB.
- **Peak RSS is not a usable no-increase metric here**: harness/`time -l` peak RSS ranged 1.85-2.61 GB (765 MB range within one split)
  because vmmap shows 464-679 MB of the writable model data swapped out (75% resident) under memory pressure. Footprint includes
  compressed/swapped pages and is stable. 000's tight RSS (~196 KB range) came from a quieter machine state and should not be
  assumed. **Use steady-state footprint (2,172 MB) plus peak footprint for 003's memory comparison, and record swap state.**
- Memory pressure may also add page-in latency to timing runs; throughput was steady in the trials, but this is an uncontrolled factor.

### 4. Stage breakdown in the original lookup-then-accumulate order (`stage_bench2`, 6 passes, timers on, replica == library on all inputs)
Full corpus (12,863; `raw/stage/all.stdout`), us per address, sum 34.77 (library 35.45 in the throughput trials; timers cost ~1 us):

| Stage | us/addr | Share |
|---|---:|---:|
| Feature lookup + weight accumulate, state features (one block per token) | 15.90 | 45.7% |
| Feature generation | 5.48 | 15.8% |
| Lookup + accumulate, prev-tag features | 3.78 | 10.9% |
| context_fill (phrase/dict search) | 3.71 | 10.7% |
| Normalize | 2.54 | 7.3% |
| CRF ctx reset + transition copy | 1.29 | 3.7% |
| Viterbi | 0.68 | 2.0% |
| Tokenize + build tokens | 0.54 | 1.5% |
| Response build | 0.49 | 1.4% |
| Labels, free temporaries, response destroy | 0.39 | 1.1% |

Lookup+accumulate in the original order is 56.6% of the time. **This supersedes 001's table**, whose shares came from the batched replica
(30.7 us/addr, ~13% faster than the library); the batched split (lookup 10.9 + accumulate 1.3 for state, 2.7 + 0.8 prev-tag) is kept in
`raw/stage/*.stdout` only as a labeled approximation. 001's label "free temporaries+response" was wrong: the response is destroyed in the
outer loop (0.07 us) and is now reported separately. development and validation give the same shape (state block 15.7 and 15.0 us).
ASCII vs non-ASCII (all, original order): 34.05 vs 35.61 us/addr total (+4.6%); normalize 2.41 vs 2.72, everything else within ~5%,
so non-ASCII input is not a major cost driver. Per address: 5.88 tokens, 45.87 state lookups (84.1% hit), 8.73 prev-tag lookups (95.3% hit).

### 5. Counter-column calibration (`tools/counter_calib.c`; window t=12-22 s; `raw/calib-summary.txt`)

| Workload | col0 | col1 | col2 | col3 |
|---|---:|---:|---:|---:|
| dependent pointer chase, 512 MiB | 3.6% | **85.0%** | 8.9% | 2.5% |
| independent ALU chains | **39.6%** | 48.0% | 8.8% | 3.5% |
| random unpredictable branches | 15.2% | 8.6% | **41.8%** | **34.4%** |
| parser steady state (001, baseline binary) | 14.9% | 60.7% | 13.6% | 10.8% |

col0 is highest for pure ALU work (Useful); col1 dominates the memory chase (back-end/processing stall); col2 and col3 both rise only on mispredicting
branches. So 001's columns 0 and 1 are confirmed; col2 vs col3 (delivery vs discarded) cannot be separated by this test and remain unconfirmed.
Correction to 001: col1 also holds 48% for the ALU loop, so "61% back-end stalled" means back-end stalled for any reason, not memory alone.
Memory-latency evidence is the locality experiment (001 section 3), not the counters. Correction appended to 001.
Raw calibration traces were deleted after export (disk full); exported tables remain.

### 6. Sub-attribution from a fresh sampling profile (all.txt, 12,846 samples; `raw/subtree-summary.txt`)
- `address_parser_features` = 14.5% of samples: `char_array_add_vjoined` 45% inclusive / 37% exclusive of it (string building via varargs,
  `strlen`, `strncmp`, appends; hot lines string_utils.c:1042, 1047), `trie_get_data` 27% (vocab lookups inside feature generation),
  `graph_has_edge` 6%, `strlen`+`strncmp` ~11%.
- `address_parser_context_fill` = 9.5%: `trie_search_tokens_from_index` 60% inclusive / 50% exclusive (trie_search.c:283 and :288 ~25% of subtree),
  `normalize_token` 22%.
So besides the CRF lookups, feature-string construction (~5% of total) and phrase-trie search (~5-6% of total) are the next targets.

### 7. Lookup detail (state-feature trie; 590,014 recorded lookups on all; development 472,695)
- Hit keys ~140-150 ns/lookup, miss keys ~220-300 ns/lookup (dev: 140-145 vs 222; runs are noisy, reps in `raw/stage/*.stdout`).
  Misses cost ~1.5x a hit although they walk about the same number of nodes.
- Trie steps per lookup: hits mean 21.0 (p50 21, p90 33, p99 47, max 76); misses mean 23.0 (p50 23, p90 28, p99 43, max 64).
  About 6.8% of hits resolve in a single step.

### 8. Cache/TLB miss counters
`xctrace record` accepts only a template name; the CPU Counters counting mode (bottleneck vs L1D-miss sampling) is fixed inside the template,
so L1D/L2/TLB miss counts could not be selected from the CLI. **Limitation, not resolved.** Cache vs TLB miss attribution is not established.

## Limitations and integrity
- Timing runs used the preserved 000 binary; `stage_bench2` links the same `libpostal.a` (hashes in `raw/hashes.txt`, source hashed before use).
- Stage/locality numbers are single-machine, single-session; throughput drifts 3-7% across trials. Stage tables are means over 6 passes.
- The first attempt to run `run_memory.sh` failed (`timeout` not present on macOS, exit 127, nothing ran) and was re-run; the first
  calibration attempt hit a full disk (chase export 0 bytes) and was re-run after space was freed. Both failed attempts' outputs were overwritten/removed.
- Development was used for tuning-oriented stage/locality work; validation timings above are baseline reference only (000's all-corpus caveat still applies).
- Swap was full throughout; memory-pressure effects on timing are not controlled.

## Decision
Baseline complete for comparing candidates. Acceptance procedure for 003: >= 5 alternating trials on development (tune) and validation/all (final),
report median and (max-min)/median, treat < ~5% as inconclusive, exact-output check on all 12,863, memory judged by steady-state footprint (2,172 MB) and
peak footprint (not RSS). Ledger, report and activity log updated: yes.
