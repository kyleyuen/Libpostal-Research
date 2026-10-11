# Experiment 155 — PLAN A4: front half without string keys (records)

Status: **halted at the pre-registered decision point.** The necessary condition passed and the hybrid failed, so A4 can pass only by replacing context fill. That needs a user decision. The pre-registration was written at 15:27:26, before any measurement; results are appended below it.

## Pre-registered amendment to A4's kill rule

This section was written before any 155 run, at the time in `raw/created.txt`.

**Why the rule is amended.**
- PLAN A4 judged the "front half" (context fill + features + lookups) against cpi's split-harness stages. In the record design, one probe also brings the weight rows. So misses that cpi books under *lookups* and *accumulate* move, and per-stage comparison is no longer like-for-like.
- A4's cold gate (≤ 1.3 µs on golden-v1) cannot be met by any design that keeps cpi's phrase search: cpi's context fill alone is 1.72 µs cold (141). A hybrid can therefore stop A4 or leave it open; it cannot pass it.

**Budgets.** These follow the gate's stated purpose of keeping **≥ 8×** reachable. All inputs are measured in ONE gated session on golden-v1 all and OSM v3 (3 rounds, median of rounds):
- O = original `bench_ext` µs; C = cpi `bench_ext` µs.
- From 119 `css` `bench_split`, cold: Σ_ntr = normalize + tokenize + response; CF = context fill; P = predict_total (block B in cpi); total = its total_parse.
- s = C / total, which converts split-harness µs to the `bench_ext` scale.
- **B** = everything after context fill: features + lookups + accumulate + Viterbi.

| Budget for B_new (cold, `bench_ext` pattern) | formula |
|---|---|
| Necessary (context fill free) | O/8 − s·Σ_ntr |
| Hybrid (cpi's context fill kept) | O/8 − s·(Σ_ntr + CF) |

**Step 3 (decisive test before porting).** Measure B_new by replaying the exact per-address record-key stream: unit, pair and own keys from O3k/O3i, in emission order, including misses. The replay uses:
- the real A2 arena (built from the model, no hot-tiering);
- hash, index probe and full string compare per distinct record key per address;
- a slot scan per emitted feature;
- K3's exact accumulate + Viterbi.

Labels must equal K0 on every address.

**Decision rules.**
- B_new_cold above the **necessary** budget on golden-v1 or OSM → **A4 stops; no port.**
- Within the necessary budget but above the **hybrid** budget → **the hybrid is a dead end.** A4 can pass only by replacing context fill, and the user is told the numbers and the size of that build before it starts.
- Within the hybrid budget → port the feature function (v2 oracle with the tokenized string, real string work, exactness against O3i on all 4 corpora).
- Warm values are reported but are not a gate.

The replay's cold numbers are optimistic: no context-fill working set competes for cache, and the feature-function control flow is free.

---

## Results (appended after the run)

### Reproduction

- Tool: `tools/rec_replay.c`, linked against the ORIGINAL `libpostal.a`, `-O3 -mcpu=native`.
  - It reuses A2's grouping code verbatim and builds the arena from the whole model: 887.8 MB including per-slot masks, 10,025,721 records.
  - The index uses a fast 8-byte-chunk hash.
  - Per address, each distinct record key is probed once (within-address reuse only). Each emitted feature, misses included, scans its record's slots in O3k order. Accumulation and Viterbi are K3's.
- Untimed smoke run: `raw/smoke/`.
- Session: `tools/run_budget.sh`, 15:29:16 onwards (`raw/budget/run.log`).
  - Contents: original `bench_ext` (20 passes), cpi `bench_ext` (20), 119 `css` `bench_split` (3), and `rec_replay` (60).
  - Corpora: golden-v1 all and OSM v3; 3 rounds, order rotated.
  - Outcome: 24/24 exit 0, 0 gate failures.
- `rec_replay` processes ran 8.6–9.8 s each, including ≈7 s of arena build and load. The **measured** replay time is ≈1–2 s, below METHOD's 10 s.
- Analysis: `tools/analyze_budget.py` → `raw/budget/analyze.stdout`.

### Exactness

The record path's labels equal K0 on **every** address in all 6 runs: 12,860 golden-v1 and 11,405 OSM addresses. Per address it probes 14.9 / 12.6 distinct records for 54.6 / 48.0 emitted keys.

### Budgets and result (µs/address, cold, `bench_ext` scale, medians of 3 rounds)

| | golden-v1 | OSM v3 |
|---|---:|---:|
| original O / cpi C (speedup) | 31.82 / 5.247 (6.06×) | 19.72 / 3.858 (5.11×; rounds 1 and 3 slow, 5.06–5.07×) |
| cpi normalize + tokenize + response (scaled) | 0.442 | 0.364 |
| cpi context fill (scaled) | 1.662 | 1.168 |
| **cpi block B** (features + lookups + accumulate + Viterbi) | **3.155** | **2.308** |
| **record design B_new** | **2.111** (−33%) | **1.509** (−35%) |
| B_new warm / cpi B warm (split harness, unscaled) | 1.438 / 1.80 | 1.251 / 1.57 |
| necessary budget, O/8 − ntr | 3.535 → **PASS** | 2.115 → **PASS** |
| hybrid budget, O/8 − ntr − cf | 1.880 → **FAIL** (by 0.23) | 1.053 → **FAIL** (by 0.46) |

All three rounds agree on both verdicts for both corpora (per-round rows in `raw/budget/analyze.stdout`).

### What the numbers imply (derived from the medians above; UNVERIFIED as end-to-end results)

The replay leaves out the feature function's control flow and string work. Both projections below are therefore **optimistic**.

**Hybrid** (cpi's context fill kept, B replaced): C − B + B_new.
- golden-v1: 4.20 µs, i.e. **≈7.6×**;
- OSM: 3.06 µs, i.e. **≈6.4×**;
- against cpi's 6.06× / 5.11× measured in the same session.

**Hard ceiling of this design.** With context fill **and** the feature control flow free, the speedup is O / (ntr + B_new): **≈12.5× golden-v1, ≈10.5× OSM**.

**What 8× requires.** Context fill plus the feature function's own work must fit in ≤ 1.42 µs on golden-v1 (cpi's context fill alone is 1.66) and ≤ 0.59 µs on OSM (cpi's is 1.17). On OSM that means cutting context fill by more than half *and* making the feature function nearly free.

**This supersedes 151's projection.** 151 projected ≈10–14× using the float32 K2 back half and an assumed front half; that is now replaced by the measured exact numbers above. The 10–14× range sits at or above this design's hard ceiling, so it was too optimistic.

### Memory of the timed format

The arena is 887.8 MB plus a 268.4 MB index, ≈1,156 MB in all, against the 1,170.7 MB of cpi structures it replaces (153). The net is ≈ −15 MB and the projected peak ≈2.53 GB (UNVERIFIED), which passes but with little headroom; see the 153 correction.

### Decision (pre-registered rule)

**Necessary condition passed; hybrid failed. The hybrid is a dead end *for ≥ 8×*.** It remains a possible gain of up to ≈+25% over cpi (≈7.6× / ≈6.4× against 6.06× / 5.11× in the same session). That figure is optimistic: the omitted feature-function control flow and string work are exactly the costs that erased 149's gain. A4 (≥ 8×) can pass only if context fill is replaced as well:
- the three phrase searches (address dictionary, component phrases, postal codes) over tries;
- prefix/suffix search;
- admin normalization.

That is the largest build in the plan. As pre-registered, **no further A4 work starts until the user has these numbers**. No port of the feature function was made.

Limitations:
- B_new is a lower bound on the real block: feature control flow and strings are excluded, and no context-fill working set competes for cache.
- The `css` split harness reads ≈4–15% above `bench_ext` (s = 0.85–0.99); all its stages are converted with s.
- OSM's cpi rounds 1 and 3 were slow, a sign of interference; the medians are reported.
