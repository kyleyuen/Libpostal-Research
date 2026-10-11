# Experiment 151 — is the current architecture optimal? Exact workload, certified-argmax feasibility, measured floors (diagnostic)

Date/time/timezone: created 2026-10-05T14:42:22-0700 (`raw/created.txt`), America/Los_Angeles. Runs 14:45–14:56 (`raw/*/run.log`).
Status: complete (diagnostic, no library change). Follow-on plan: [`PLAN.md`](PLAN.md).

## Question and hypothesis

User request (2026-10-05): 150 confirmed ≈6× on an unseen holdout, and tuning `cpi` further has stalled. Until now every experiment asked how
to make the current design faster. This one asks whether the design itself is the fastest possible, using math and measurement, and how far
`cpi` is from the best achievable. If the answer is no, write an audit plan.

Hypotheses (written before running):
- **H1.** REWRITE_MATH §6 and 141 treat ≈0.8–0.9 µs of warm accumulate + Viterbi as "fixed by byte-identity", because reordering float sums
  changes the bits of the scores. But the output is the label sequence, not the scores. If the best Viterbi path wins by a margin far larger than
  any rounding error, then reordered or float32 arithmetic gives the same labels, and the original order is only needed as a fallback.
  Prediction: few or no real addresses need a fallback.
- **H2.** LIMITS (10-02) never measured the arithmetic per address ("9–16 entries per row, UNVERIFIED"). It can be counted exactly from the 142
  oracles plus the model.
- **H3.** Most of `cpi`'s remaining time is the front half and memory traffic caused by the *design* (one string key → dictionary probe → row per
  feature, plus trie walks). It is not caused by the CRF arithmetic.

Change: none to the library. Four new tools, all linked against the preserved ORIGINAL `000-baseline/artifacts/libpostal.a`:

| Tool | What it does |
|---|---|
| `tools/oracle_bound.c` | Rebuilds every state row and state-transition row from O3i in the original loop order and requires bit-identity with O4. Runs the library's own `crf_context_viterbi` on the O4 rows. Counts work and cache lines. Certified-argmax test (§3). |
| `tools/sim_layout.c` | Replays the exact key/id stream (O3k + O3i) through two memory layouts: today's "probe + row" and a "compiled lexicon record" layout. |
| `tools/ubench.c` | Machine constants: clock, FADD latency, vector FADD throughput. |
| `tools/replay_kernel.c` | Timed floor of the back half (accumulate + Viterbi) with the feature ids given for free: K0 original CSR/f64 + library Viterbi; K1 packed co-located f64 rows (exact order); K2 certified float32 with exact fallback. |

Plus `tools/run_ic.sh` + `tools/analyze_ic.py`: instructions and cycles per address of the original and `cpi` (029's difference method).
Plus `tools/run_same.sh` + `tools/analyze_same.py` (added after review): the original, `cpi` and the replay kernel in **one gated session on the same corpora**,
golden-v1 all (`profiling/golden-v2-novel/v1-all.tsv`, the oracle corpus) and OSM v3. 3 rotated rounds, 18/18 runs exit 0, 0 gate failures.
Every multiplier in the Answer section uses these denominators.

## Reproduction

- Host: Apple M4 Pro, macOS Darwin 25.5.0, Apple clang 17.0.0, AC power (`raw/ubench.power.txt`), load average ≈2.7–3.2 (interactive session open).
- Root commit `181737d3`. Base library: preserved original `libpostal.a`; its hash and the model hash are in `raw/bound/hashes.txt`.
- Oracles: `research/oracles/v1/{golden-v1,novel,osm-v3,fuzz-s1-200k}` (142; read only).
- Commands:
  - `tools/run_bound.sh`, `tools/run_sim.sh`;
  - `tools/run_ic.sh`: gated, serial, 3 rotated reps;
  - `tools/run_replay.sh`: gated, serial, 3 rounds, `-O3 -mcpu=native`;
  - `clang -O2 tools/ubench.c`.

  All final builds exit 0; all 24 counter runs and 6 replay runs exit 0; 0 gate failures (`raw/ic/gate.log`, `raw/replay/gate.log`).
- Failed attempts, kept:
  - `raw/attempt1_build_fail`: `LIBPOSTAL_DATA_DIR` not defined.
  - `raw/attempt2_fuzz_t0`: on fuzz, a token had O4 rows but no O3i lines; exit 4.
  - `raw/attempt3_fuzz_desync`: addresses with O4 but no O3i desynchronized the two streams on fuzz only. This reported 347,902 "addresses" and spurious mismatches.

  The golden-v1/novel/OSM results were identical in attempts 2, 3 and the final run.

## Results

### 1. The work model is the original's real work (self-check)

| Corpus | predicted addresses | state-row bit mismatches | state-trans bit mismatches | label disagreements with library Viterbi |
|---|---:|---:|---:|---:|
| golden-v1 | 12,860 | 0 | 0 | 0 |
| novel | 12,854 | 0 | 0 | 0 |
| OSM v3 | 11,405 | 0 | 0 | 0 |
| fuzz seed 1 (200k inputs) | 177,109 | 0 | 0 | 0 |

### 2. Exact workload per address (`raw/bound/*.stdout`; replaces LIMITS' assumptions)

| | golden-v1 | novel | OSM | fuzz |
|---|---:|---:|---:|---:|
| tokens T | 5.88 | 5.74 | 5.36 | 7.69 |
| state keys / hits | 45.9 / 38.6 | 46.9 / 38.6 | 40.2 / 35.9 | 66.8 / 54.4 |
| state weight adds | 478 | 473 | 454 | 661 |
| state-trans (prev-tag) keys / hits | 8.7 / 8.3 | 8.6 / 8.2 | 7.8 / 7.5 | 12.4 / 11.9 |
| state-trans weight adds | 614 | 580 | 587 | 957 |
| Viterbi cells (T−1)·L², L = 20 | ≈1,953 | ≈1,895 | ≈1,745 | — |
| distinct 128-B weight lines: original CSR / packed f64 / packed f32 | 171 / 104 / 71 | 169 / 101 / 70 | 162 / 99 / 67 | 209 / 128 / 87 |

Model facts:
- 11,470,623 state rows, mean 2.95 nonzeros; 2,086,474 state-trans rows, mean 6.4.
- The rows actually hit are far denser: 12.4 nonzeros per state hit, ≈74 per state-trans hit. A full 20-label row is the most common state hit.
- **LIMITS assumed 420–750 adds per address; the measured figure is ≈1,090.**

### 3. Certified argmax: float order is NOT pinned by byte-identity (H1 confirmed)

**Method.** For each score, a rigorous bound covers the alternative arithmetic's error plus the original's own rounding (γ_k·Σ|w|):
- E1: double precision in any summation order;
- E2: float32 weights and sums;
- E3: float32 everything, with a conservative extra term for DP rounding.

The bounds are widened ×2 so they cover every runtime interval. They are propagated through the original DP with monotone round-to-nearest
operations. An address is certified only if **every decision on the original path is strictly separated**; otherwise it falls back to the
original order. Exact ties always fall back.

| Corpus | exact ties on path | certified E1 (f64, any order) | E2 (f32 sums) | E3 (f32 all) |
|---|---:|---:|---:|---:|
| golden-v1 | 0 | 100% | 100% | 100% |
| novel | 0 | 100% | 100% | 100% |
| OSM v3 | 0 | 100% | 100% | 100% |
| fuzz | 15 | 99.990% | 99.989% | 99.943% |

So the labels can be computed in any order, or in float32, with an exact fallback, and still be byte-identical. **REWRITE_MATH §6 ("exact
algebraic restructuring ruled out") is corrected by this result**; a dated note was appended there.

### 4. Memory layout simulation (`raw/sim/*.stdout`; feature + weight stage only, context-fill tries excluded)

| per address | golden-v1 | novel | OSM |
|---|---:|---:|---:|
| today's design (P): key probes + rows read | 54.6 + 46.9 | 55.6 + 46.8 | 48.0 + 43.5 |
| P: distinct lines | 152 | 152 | 142 |
| compiled lexicon records (C): records probed | **16.6** | 18.6 | 14.3 |
| C: distinct lines incl. index | **81** | 83 | 74 |

- A record holds every row keyed on one word or phrase (word, prev/next word, first word, P word/prev word, phrase*, commonly*). Bigram records serve both `prev word+word` and `word+next word`. Values are f32, which §3 makes legal.
- The compiled layout touches **≈47% fewer lines** and makes **≈6× fewer independent random accesses** (16.6 vs 101.5). That is before counting the context-fill trie walks, which 045 measured at ≈104 more lines per address in `l3`.
- The LRU columns in `raw/sim` read 0 whenever the corpus footprint fits the capacity, because a cyclic scan is LRU's degenerate case. They are not informative and are not used.

### 5. Machine constants and `cpi`'s instruction budget

`raw/ubench.stdout`, median of 9:
- effective clock **4.47 GHz** (LIMITS assumed 4.51; 029 inferred ≈4.1);
- dependent f64 FADD **≈2.5 cycles**;
- independent 128-bit FADD ≥ 2.67 per cycle. This is a lower bound, because 8 chains are still latency-limited.

`raw/ic/analyze.stdout`: difference method, 3 reps. These are whole-process counts, so the spread is wide.

| | instructions / address | cycles / address | IPC |
|---|---:|---:|---:|
| original, golden-v1 dev | 210,670 [200.6k–211.1k] | 144,480 [141.6k–153.5k] | 1.46 |
| `cpi`, golden-v1 dev | **50,518** [48.6k–52.2k] | **21,343** [17.6k–26.1k] | 2.37 |
| original, OSM | 189,330 [184.6k–189.8k] | 89,308 [76.4k–89.5k] | 2.12 |
| `cpi`, OSM | 45,393 [37.7k–45.6k] | 12,416 [9.4k–17.8k] | 3.66 |

`cpi` removed 4.2× of the instructions and ≈6.8× of the cycles on dev, but it still executes ≈50k instructions per address. The CRF arithmetic
in §2 is ≈1,090 adds plus ≈5,900 Viterbi add/compare operations, i.e. ≈7k scalar or ≈2k 4-wide vector operations. The model's arithmetic is a *lower* bound on the instructions it needs, because loads, indexing and tokenization add more. So **up to ≈85% of
`cpi`'s instructions** are work the model does not require: key building, probing, trie walking and bookkeeping. The exact share is not derived.

### 6. Measured back-half floor (`raw/replay/*.stdout`; ids free, real model rows)

Each kernel's labels equal the library's on every address (0 mismatches, 3 rounds). K2's runtime certificate is cheaper and cruder than §3's,
so it fell back on 6 golden-v1 addresses (0.047%) and on 0 OSM addresses. The K1 fallback handled all of them exactly.

| µs / address (median of 3 rounds) | golden-v1 cold | golden-v1 warm | OSM cold | OSM warm |
|---|---:|---:|---:|---:|
| K0 original CSR f64 + library Viterbi | 3.45 | 1.90 | 2.29 | 1.78 |
| K1 packed co-located f64, exact order | 2.39 | 1.06 | 1.45 | 1.05 |
| K2 certified f32 + fallback | **1.77** | **0.75** | **0.99** | **0.78** |
| for context: `cpi` accumulate + Viterbi (141 split harness) | 1.28 | 0.90 | 1.03 | 0.83 |

Cold is the median of passes 2–12 in corpus order. Warm is the marginal cost of an immediate repeat.

Caveats:
- Each process ran ≈5 s, below METHOD's 10-s trial minimum.
- The warm difference method is noisy; one OSM K0 round has a negative low end.
- Round 1 of golden-v1 K2 (2.51 cold) is an outlier.
- `cpi`'s row prefetch books part of the row stall under "lookups", so its 1.28 is not a like-for-like stage.

**Same session, same corpora** (`raw/same/analyze.stdout`; median of 3 rounds; ratios formed within each round):

| µs / address, cold (`bench_ext` pattern) | golden-v1 all | OSM v3 |
|---|---:|---:|
| original (preserved baseline) | 34.79 | 20.91 |
| `cpi` | 5.56 (**6.26×**) | 3.66 (**5.71×**) |
| K0 / K1 / K2 back half only | 3.92 / 2.44 / **2.03** | 2.39 / 1.37 / **0.99** |
| K2 warm | 0.66 | 0.74 |

The replay kernel's cold numbers are optimistic. The front half's data is absent, so the kernel sees less cache pressure than a full parser.

**Reading.** The back half is *not* where `cpi`'s gap is. Certified f32 alone saves only ≈0.1–0.15 µs warm against `cpi`'s fixed bucket.

## Answer: is the current solution the most optimal? No

**Where `cpi` stands** (same session, golden-v1 all / OSM): 5.56 / 3.66 µs cold, 6.26× / 5.71× over the original.
141's stage split (dev) puts 60–68% of cold time in the front half (context fill + feature keys + lookups) and ≈2.7 µs in cold−warm memory stall.

**Floors computed by one method for both architectures** (golden-v1). Both use the measured workload (§2), the 4.47 GHz clock (§5),
λ = 127 ns and M ≥ 29.7 (014). These are DERIVED limits, not predictions:

| term | current architecture (string keys + probes + tries) | alternative (compiled records + certified argmax) |
|---|---:|---:|
| CRF arithmetic (≈1,090 scatter adds + Viterbi ≈2k vector ops) | ≈0.36 µs | ≈0.36 µs |
| other compute | ≈4.7k instr (55 key builds + hashes + trie walks) → +≈0.26 µs | ≈700 instr → +≈0.04 µs |
| distinct lines per address | ≈286: 152 feature/weight (§4) + ≈134 trie, vocab, role, postcode and dictionary (045) | ≈81–100: 81 (§4) + phrase probes |
| memory throughput N·λ/M | ≈1.22 µs | ≈0.35–0.43 µs |
| dependent chain D·λ | D≈4 → ≈0.51 µs | D=2 → ≈0.25 µs |
| **floor band** (perfect overlap … no overlap) | **≈1.2–2.3 µs** | **≈0.43–1.1 µs** |
| original ÷ floor (34.79 µs, same session) | ≈15–29× | ≈32–81× |
| `cpi` ÷ floor (5.56 µs) | `cpi` is ≈2.4–4.6× above it | `cpi` is ≈5–13× above it |

> **Correction, 2026-10-05 (experiments 153–154).** The alternative column above assumes float32 4-lane compute (0.36 µs) and f32 record lines (81). Both are ruled out:
> - A2 (153) rejected float32 on memory.
> - A3 (154) puts the exact f64 compute floor at ≈0.6–0.75 µs.
> - f64 records touch ≈1.46× more weight lines (packed f64 / f32 = 104 / 71, §2), about 109 lines, which is ≈0.47 µs at the same λ and M.
>
> The exact alternative's floor band is therefore **≈0.65 µs (perfect overlap) to ≈1.5 µs (no overlap)**. Its ceiling against the same-session original (34.79 µs) is **≈23–54×**, not 32–81×, and `cpi` sits ≈3.7–8.6× above it. The measured block after context fill (155: 2.11 µs on golden-v1) is ≈2–3× above this floor. The text below is kept as written.

The alternative's floor is ≈2–3× lower than the current architecture's, and the gap comes entirely from memory and probe traffic (lines and chain
depth). The arithmetic is identical in both. Within its own architecture, `cpi` is within ≈2.4–4.6× of the limit. LIMITS (10-02) expected optimized
code to stop paying off at "a few times its floor", which is where `cpi` sits.

> **Superseded 2026-10-05 by experiment 155.** The exact record design's block after context fill was measured at 2.11 µs (golden-v1) and 1.51 µs (OSM). This gives a hard ceiling of ≈12.5× / ≈10.5× even with a free context fill, and ≈7.6× / ≈6.4× (optimistic) if cpi's context fill is kept. The 10–14× projection below is kept for the record; it was too optimistic.

**Realistic projection for the alternative (UNVERIFIED; this is a prediction, not a measured improvement, and must not appear in the professor
report as a result).** The parts are:
- K2's same-session back half: 2.03 µs cold on golden-v1, 0.99 µs on OSM;
- a front half of 0.3–0.8 µs (assumption);
- index and phrase misses of 0.2–0.5 µs.

That totals ≈2.5–3.3 µs on golden-v1 and ≈1.4–2.2 µs on OSM, i.e. **≈10–14× golden-v1 and ≈9–14× OSM** against the same-session original.
PLAN A4 is the test.

**Why the current design caps out.** It inherits libpostal's structure:
- every feature is a **string** key built per token (≈55 per address);
- each key is probed in a feature dictionary, then a weight row is read;
- context fill walks three tries;
- scores are accumulated by scalar double scatter into a dense L×L matrix that is zeroed per token.

Two rejected tracks show the remaining cost is structural (random accesses and instructions per address), not arithmetic:
- 146–149: integer keys *inside* this design gave no gain, because deriving them costs as much as the strings;
- 139/140: interleaving addresses was slower.

## Codebase audit findings (no code changed)

1. **Open defect (from 150).** `cpi` aborts on `libpostal_parser_print_features(true)`. `research/final/119-cpi/sources/crf.c:793` uses `EXP_ROW_PACK`, which supports only the direct scoring path. The original exits 0.
2. **Load time.** 11.5 s vs 2.7 s (150). It breaks even only after ≈389k addresses per process. Any compiled-lexicon build must happen offline or be counted here.
3. **Memory headroom.**
   - Peak is only ≈71 MB under the original (2.543 vs 2.614 GB); steady headroom is ≈562 MB.
   - Packed records take 510 MB in f64 and 292 MB in f32, against 620 MB for the CSR arrays.
   - f32 records *plus* f64 rows kept for fallback fit only if the records replace the CSR arrays. The plan must prove the peak before building.
4. **Other assert-style aborts in `cpi` sources** (reachability not tested, except item 1):
   - `features.c:26/71/113` abort when a call passes more than 8 (or 1–3) named parts. The call sites pass fixed counts, so this is UNVERIFIED as unreachable.
   - `crf_context.c:762` (`EXP_VITERBI_MAXONLY`, compiled into `cpi`) aborts if no label matches the maximum. That is only possible with non-finite scores. The original would continue instead.
   - The `*_VERIFY` aborts are not compiled into `cpi` (`switches.txt`).
5. **API coverage.** The parser exports are `libpostal_setup_parser`, `_setup_parser_datadir`, `_teardown_parser`, `_parse_address`, `_get_address_parser_default_options`, `_address_parser_response_destroy` and `_parser_print_features` (`libpostal.h:162–272`). 150's probe covers every one except **`libpostal_setup_parser_datadir`**, which is untested on `cpi`.
6. **Analysis premise corrected.** REWRITE_MATH §6 and 141's "fixed by byte-identity" bucket assumed float order was pinned; §3 removes that assumption. The earlier "7.8× not reachable" conclusion was about *the current architecture*, and it does not bound the alternative.

## Limitations

- **The floor is a model.** It uses measured counts and constants, but overlap assumptions bracket it (≈0.43–1.1 µs). It excludes tokenizer edge cases, non-ASCII normalization and response building, which take ≈0.29 µs warm in `cpi` (141 "other").
- **The certificate and fallback have a gap.** §3 is rigorous for the bounds stated, using the widened-interval argument. K2's runtime certificate is a cruder, conservative sketch, not a written proof. A production certificate needs that proof and fuzz at scale (PLAN A1).
- **The layout simulation is incomplete.**
  - Record sizes count only the rows each address uses, so they are a lower bound. Arena placement is pseudo-random.
  - Phrase matching (context fill) is not modelled for layout C.
  - A front half that yields the same feature ids without string keys is unproven, and it is the hardest part (PLAN A4).
- **The replay kernel's cold numbers are optimistic** (no front-half working set competing for cache).
- **The replay kernel has data and duration limits.** It ran on golden-v1 and OSM only, with trials shorter than 10 s and an interactive load (load average ≈3).
- **The counter numbers are noisy.** The whole-process difference method spreads ±10–40% on cycles.
- **The fuzz corpus is the only source of exact ties.** Fallback is therefore required in production, not optional.

## Decision

Diagnostic complete. **Answer: no.** `cpi` is a well-tuned implementation of an architecture whose own floor is higher than the alternative's,
and it sits ≈5–13× above the floor of a compiled-records + certified-argmax design. That design keeps the same model, same Viterbi,
byte-identical output and one thread.

The audit plan is [`PLAN.md`](PLAN.md), with milestones and kill rules; nothing in it is built. `cpi` (121) remains the deliverable.

Ledger, activity log, the REWRITE_MATH §9 correction and the handoff §13 note were updated 2026-10-05. The professor report keeps every speed figure unchanged.
It gained one dated correction note: its "fixed by byte-identity" point is superseded by the verified certification result, and no new speed is claimed.
