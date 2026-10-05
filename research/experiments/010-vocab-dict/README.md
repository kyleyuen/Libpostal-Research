# Experiment 010 — Vocabulary dictionary, fast feature join, lean Viterbi context, arena prefetch (on top of 009)

Date: 2026-09-29 PDT (session 19:52-20:07). Status: **accepted (variant `g`: 2.37x golden-v1 / 2.40x novel vs the original baseline; byte-identical; single thread; peak memory lower)**
Run unattended (laptop caffeinated, AC gate with wait-and-retry).

## Changes (each behind its own compile switch for attribution; all on top of 009 `f`)
- **EXP_FAST_JOIN** (`features.c`): `feature_array_add` computes the piece lengths once, reserves capacity once (same doubling policy) and copies short pieces inline instead of calling `memcpy`/`strlen`-heavy `char_array_add_vjoined` (8 of the profile's biggest libc callers). Same bytes.
- **EXP_LEAN_CONTEXT** (`crf.c`): a Viterbi-only replacement for `crf_context_set_num_items`. The library version resizes and zeroes 8 matrices per address (four of them marginal arrays Viterbi never reads) and, because `double_matrix_resize` compares against the *current* dimensions, reallocates whenever an address is longer than the previous one. The lean version tracks real capacity and zeroes only `state`, `state_trans` and `backward_edges`.
- **EXP_ARENA_PREFETCH** (`crf.c`): prefetch each candidate key's arena entry before comparing. **No effect** (see below).
- **EXP_VOCAB_DICT** (`address_parser.c`): the vocabulary trie (`word_vocab_frequency`, up to 3 lookups per word) is converted at load into the same exact dictionary with the count stored beside the key (`fd_build_ex(trie, split=false, wide_value=true)`); the trie is freed.
`g` = all four. `f0` = 009's `f` rebuilt from the 010 sources with no new switch (rebuild-noise control).

## Vocabulary dictionary prototype gate (measurement only, `tools/vocab_proto.c`, `raw/vocab_proto.stdout`)
```
vocab trie: nodes=7147311 tail=12484341 data=3694561 keys(header)=3694560 alphabet=256 bytes=94.6 MB
dict: keys=3694560 leaves_total=3694560 visited=3694560 skipped=0 build=0.70s table=37.6 MB (4926082 slots) arena=58.9 MB (mean 16.73 B/key) total=96.5 MB = 102.0% of trie
exhaustive round-trip: checked=3694560 mismatches=0
query words: 237861
fuzz: mutations=2000000 (trie hits 844006) mismatches=0
timing corpus-order rep0: trie 48.4 ns/lookup | dict 31.4 ns/lookup (hits 209536/209536)
timing shuffled rep0: trie 88.9 ns/lookup | dict 47.9 ns/lookup (hits 209536/209536)
timing corpus-order rep1: trie 48.1 ns/lookup | dict 32.2 ns/lookup (hits 209536/209536)
timing shuffled rep1: trie 88.1 ns/lookup | dict 47.0 ns/lookup (hits 209536/209536)
timing corpus-order rep2: trie 55.3 ns/lookup | dict 29.8 ns/lookup (hits 209536/209536)
timing shuffled rep2: trie 82.3 ns/lookup | dict 46.7 ns/lookup (hits 209536/209536)
```
Exact (3,694,560 keys round-trip, 2M fuzz mutations, 0 mismatches). Size 96.5 MB vs 94.6 MB for the trie (+1.9 MB; the total across all converted structures is still 19 MB smaller than the tries they replace).
Lookup 32 ns vs 48 ns (corpus order), 47 vs 88 ns (shuffled). I predicted about a 2% end-to-end gain from that; the measured gain was 6.7% (below).

## Correctness (hard constraints)
- **Byte-identical**: `f0`, `fJ`, `fC`, `fE`, `fV`, `g` match the baseline on golden-v1/all (12,863) and the novel set (12,863) (`raw/outputs_*`). Conversion logs confirm all three dictionaries are active in `g`.
- **Single thread**: no thread primitives; CPU time equals wall time in every run (the estimator table shows median = CPU-time median).
- **Memory (no increase)**: see peak-footprint table; `fV` and `g` are ~2.1% BELOW the baseline.

## Results (controlled unattended session; baseline, 009 `f`, `f0`, `fJ`, `fC`, `fE`, `fV`, `g`; 5 processes x 19 timed passes per variant per dataset; variants rotated)
| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 95 | 2.92 | nan | 100% | 36.78 | 36.17 | 36.77 | nan (0) | 37.04 |
| novel | f | 95 | 5.96 | nan | 100% | 17.41 | 17.24 | 17.40 | nan (0) | 17.52 |
| novel | f0 | 95 | 5.93 | nan | 100% | 17.52 | 17.33 | 17.52 | nan (0) | 17.66 |
| novel | fJ | 95 | 5.94 | nan | 99% | 16.73 | 16.54 | 16.73 | 16.34 (1) | 16.86 |
| novel | fC | 95 | 5.98 | nan | 100% | 17.14 | 16.96 | 17.13 | nan (0) | 17.36 |
| novel | fE | 95 | 5.96 | nan | 92% | 17.53 | 17.24 | 17.52 | 17.00 (8) | 17.82 |
| novel | fV | 95 | 6.65 | nan | 99% | 16.37 | 16.19 | 16.37 | 16.11 (1) | 16.55 |
| novel | g | 95 | 6.74 | nan | 97% | 15.34 | 15.15 | 15.34 | 15.28 (3) | 15.59 |
| v1-all | baseline | 95 | 2.92 | nan | 97% | 36.04 | 35.27 | 36.02 | 35.43 (3) | 35.79 |
| v1-all | f | 95 | 5.97 | nan | 99% | 17.42 | 17.23 | 17.42 | 16.89 (1) | 17.46 |
| v1-all | f0 | 95 | 5.96 | nan | 98% | 17.49 | 17.33 | 17.48 | 17.34 (2) | 17.52 |
| v1-all | fJ | 95 | 6.06 | nan | 100% | 16.79 | 16.66 | 16.79 | nan (0) | 17.02 |
| v1-all | fC | 95 | 5.96 | nan | 97% | 17.11 | 16.97 | 17.11 | 16.69 (3) | 17.14 |
| v1-all | fE | 95 | 5.95 | nan | 95% | 17.46 | 17.31 | 17.46 | 17.34 (5) | 17.66 |
| v1-all | fV | 95 | 6.65 | nan | 96% | 16.33 | 16.12 | 16.32 | 16.10 (4) | 16.48 |
| v1-all | g | 95 | 6.76 | nan | 82% | 15.18 | 14.89 | 15.18 | 14.99 (17) | 15.31 |


Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | f | 2.113x | 2.098x | 2.113x | nanx | 2.114x |
| novel | f0 | 2.099x | 2.087x | 2.099x | nanx | 2.097x |
| novel | fJ | 2.198x | 2.187x | 2.199x | nanx | 2.197x |
| novel | fC | 2.146x | 2.132x | 2.147x | nanx | 2.134x |
| novel | fE | 2.098x | 2.098x | 2.099x | nanx | 2.079x |
| novel | fV | 2.247x | 2.234x | 2.247x | nanx | 2.238x |
| novel | g | 2.397x | 2.388x | 2.397x | nanx | 2.376x |
| v1-all | f | 2.069x | 2.047x | 2.068x | 2.098x | 2.049x |
| v1-all | f0 | 2.061x | 2.036x | 2.060x | 2.043x | 2.043x |
| v1-all | fJ | 2.146x | 2.117x | 2.145x | nanx | 2.103x |
| v1-all | fC | 2.106x | 2.078x | 2.105x | 2.123x | 2.088x |
| v1-all | fE | 2.064x | 2.038x | 2.063x | 2.043x | 2.026x |
| v1-all | fV | 2.208x | 2.188x | 2.206x | 2.201x | 2.171x |
| v1-all | g | 2.374x | 2.369x | 2.372x | 2.364x | 2.337x |



Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   f     : countries=17 min 1.960 (lt) median 2.095 max 2.276 (ru); below 1.05x: 0
  novel   f0    : countries=17 min 1.931 (lt) median 2.072 max 2.248 (ru); below 1.05x: 0
  novel   fJ    : countries=17 min 2.050 (lt) median 2.179 max 2.348 (ru); below 1.05x: 0
  novel   fC    : countries=17 min 2.002 (lt) median 2.124 max 2.317 (ru); below 1.05x: 0
  novel   fE    : countries=17 min 1.944 (lt) median 2.074 max 2.257 (ru); below 1.05x: 0
  novel   fV    : countries=17 min 2.038 (tw) median 2.256 max 2.516 (ru); below 1.05x: 0
  novel   g     : countries=17 min 2.214 (lt) median 2.377 max 2.649 (ru); below 1.05x: 0
  v1-all  f     : countries=17 min 1.863 (lt) median 2.056 max 2.196 (ru); below 1.05x: 0
  v1-all  f0    : countries=17 min 1.867 (nl) median 2.045 max 2.171 (ru); below 1.05x: 0
  v1-all  fJ    : countries=17 min 1.936 (nl) median 2.125 max 2.252 (gb); below 1.05x: 0
  v1-all  fC    : countries=17 min 1.866 (lt) median 2.076 max 2.273 (tw); below 1.05x: 0
  v1-all  fE    : countries=17 min 1.852 (lt) median 2.040 max 2.162 (ru); below 1.05x: 0
  v1-all  fV    : countries=17 min 1.999 (lt) median 2.177 max 2.438 (ru); below 1.05x: 0
  v1-all  g     : countries=17 min 2.111 (lt) median 2.356 max 2.544 (ru); below 1.05x: 0


Peak memory footprint per process (bytes) by variant (all processes, both datasets):

| variant | processes | min | median | max | max RSS max |
|---|---:|---:|---:|---:|---:|
| baseline | 10 | 2,613,151,952 | 2,613,463,260 | 2,614,331,600 | 2,615,181,312 |
| f | 10 | 2,601,339,112 | 2,601,920,732 | 2,603,026,640 | 2,603,892,736 |
| f0 | 10 | 2,602,043,600 | 2,602,330,320 | 2,603,649,256 | 2,604,449,792 |
| fJ | 10 | 2,601,945,320 | 2,602,256,604 | 2,602,830,056 | 2,603,663,360 |
| fC | 10 | 2,600,749,288 | 2,600,986,844 | 2,602,109,136 | 2,602,975,232 |
| fE | 10 | 2,601,502,952 | 2,602,395,868 | 2,604,206,336 | 2,604,990,464 |
| fV | 10 | 2,558,101,664 | 2,558,503,096 | 2,559,494,304 | 2,560,360,448 |
| g | 10 | 2,557,266,104 | 2,557,470,892 | 2,558,068,896 | 2,558,935,040 |



Gain of each switch over 009 `f` (golden-v1 / novel, by median): `fJ` +3.7% / +4.0%, `fC` +1.8% / +1.6%, `fE` -0.2% / -0.7% (no effect: dropped in later builds), `fV` +6.7% / +6.4%, all four `g` +14.7% / +13.5%.
The rebuild control `f0` sits within 0.4% of `f`, which sets the noise floor: the `fC` and `fE` deltas are near it, `fJ` and `fV` are clearly above it.

## Interpretation and limitations
- The vocabulary gain was three times my estimate. A likely reason (unverified) is that the vocabulary trie's lookups were cold in a way the microbenchmark on recorded words did not reproduce.
- The vocabulary conversion lowers PEAK footprint by ~55 MB (2.557 GB vs 2.613 GB baseline); the cause is not established (a load-time transient in the vocabulary path is the leading suspect).
- Setup time: 2.9 s baseline -> ~6.7 s (+3.8 s: feature tries 3.1 s, vocabulary 0.7 s).
- `fE` adds code without benefit and is left out of the 011 build.
- Shared, loaded machine (about 100% of passes saw an involuntary context switch); variants were interleaved and the P10, CPU-time and median estimators agree within ~1%.

## Decision
**Accepted** as the new best (`g`); 011 builds on it. Ledger, report and activity log updated: yes.
