# Plan — integer feature pipeline (rewrite track) for the libpostal parser

Status: **PLAN ONLY, awaiting the user's approval.** Written 2026-10-04 after experiment 141. Nothing below has been built.
Every speed or memory figure that does not cite an experiment is an **UNVERIFIED** estimate. All hard rules of `research/HANDOFF_2026-10-04.md` §2 and `research/METHOD.md` apply without exception.
Base deliverable: `cpi` (`research/final/119-cpi`). Origin: HANDOFF §11.

## 1. Goal, and what "done" means

Replace the parser's front half (context fill → feature strings → string lookup) with a pipeline in which context fill and feature generation emit **model feature ids directly**. No string key reaches the lookup. Accumulation, Viterbi and response stay as in `cpi`.

A milestone candidate is done when all of the following hold:
- byte-identical on everything `tools/verify.sh` covers, plus the stage oracles (§3);
- one thread;
- peak and steady footprint ≤ the original's;
- faster than `cpi` under the 121/103 protocol;
- not slower on a never-used OSM set (§2.6).

**The 7.8× target is not promised** (§2 explains why).

## 2. Sizing from experiment 141

Measured on the clean-ish machine, 2026-10-04 19:01–19:12. Details and caveats are in `experiments/141-warm-bound-cpi/README.md`.

| | dev | OSM | unseen v11 |
|---|---:|---:|---:|
| original bench_ext µs | 31.14 | 18.43 | 18.23 |
| cpi bench_ext µs | 5.10 | 3.33 (121; 141's 3.80 was contaminated) | 3.27 |
| cpi warm bound µs (088 method) | 2.42 | 2.13 | 2.15 |
| **warm bound speedup** | **12.9×** | **8.6×** | **8.5×** |
| 7.8× needs µs (same-session original / 7.8) | 3.99 (−22%; 141) | 2.48 (−25.7%; 121: 19.32 / 7.8 vs cpi 3.334) | 2.34 (−28.5%; 141) |
| front half, cold / warm µs (split harness) | 3.77 / 1.26 | 2.28 / 1.08 | 2.07 / 1.10 |
| of which features + lookups, cold / warm | 2.04 / 0.89 | 1.29 / 0.73 | 1.23 / 0.77 |
| fixed (accumulate + Viterbi), cold / warm | 1.28 / 0.90 | 1.03 / 0.83 | 0.97 / 0.80 |
| other (normalize + tokenize + response), cold | 0.50 | 0.40 | 0.37 |

What the table implies:

- **Floor.** About 0.8–0.9 µs of warm compute per address (accumulate + Viterbi) is pinned by byte-identity. No front-half rewrite touches it.
- **What 7.8× requires.** Each percentage comes from one session's original and cpi; sessions are never mixed. In split-harness proportions, if nothing else changes:
  - **dev:** ≈1.22 µs removed (32% of the front half, 60% of features + lookups);
  - **OSM:** ≈0.96 µs (42% of the front half, 74% of features + lookups);
  - **u11:** ≈0.98 µs (47%, 80%).
- **What the integer pipeline can reach.** Its direct target is *features + lookups*: 1.29 / 1.23 µs cold on OSM / u11. With context fill unchanged, 7.8× needs 74% (OSM) to 80% (u11) of those µs removed. That is unlikely (UNVERIFIED), because id generation still costs table probes and the rare-word n-gram paths stay on strings.
- **Empirical anchor.** 119 turned ≈13.5 string keys per address into ids and gained +2.7% v1 / +5.6% OSM. About 17.5 (dev) / 14.1 (OSM) string keys per address remain (123). A naive extrapolation gives a few percent to ≈+10% for converting them all.
  - The rewrite's case rests on doing better than that extrapolation: removing the *string-building* cost in features as well, which 122 showed is where bigram time goes. **UNVERIFIED.**
- **Expectation (UNVERIFIED; the 40–60% removal share is an assumption, not a measurement).** If the pipeline removes 40–60% of features + lookups cold:
  - **OSM:** that is 0.52–0.77 of 3.74 split-µs, i.e. −14…−21%. Applied to 121's clean numbers (original 19.32, cpi 3.334 µs) it gives 2.63–2.87 µs, **≈6.7–7.3× OSM**. 7.8× needs −25.7%.
  - **dev:** 0.82–1.22 of 5.57 split-µs, i.e. −15…−22%, gives **≈7.2–7.8× dev**. 7.8× needs −22%, so it is reachable only at the top of the range.
  - **The 119 anchor points to the bottom of this range or below.** Reaching 7.8× on OSM-style data would additionally need context-fill cards (C6–C7) to work.

## 3. Stage-level oracles (from the ORIGINAL library)

### 3.1 Why

An end-to-end mismatch in a three-stage rewrite says only "something differs". Per-stage dumps from the original library pin every failure to one stage, address and token. This is the "partial oracle" from the parallel-Claude C compiler (HANDOFF §11.2 item 3).

### 3.2 The oracle build (milestone M0, after approval; it counts as library work)

- **Sources.** The original libpostal sources at the pinned checkout plus one patch `EXP_ORACLE`, applied in a new experiment directory (never in `libpostal/src`). The patch only adds `fprintf` after a stage finishes; it changes no computation.
- **Build.** Plain `-O2`, no PGO, no LTO, original allocator. Never timed.
- **Identity control (before any oracle is trusted).** With dumping compiled in and switched on, `dump_outputs` gives:
  - golden-v1 all sha `45574ee2…`;
  - novel sha `806b8a81…`;
  - OSM identical;
  - a match against `profiling/golden-v1/baseline-outputs.jsonl`.
- **Negative control.** For each oracle, a deliberately perturbed copy must make the comparator fail at the right address and token. The perturbations are: one id swapped in one token, one score bit flipped, one token boundary moved. The perturbed files are kept in `raw/`.
- **Corpora.** golden-v1 all (12,863), novel (12,863), OSM v3 (11,405), and the first 200,000 addresses of fuzz seed 1. The 2 × 2M fuzz inputs stay end-to-end only, because of size. golden-v12+ unseen sets are never dumped before their single use.
- **Storage.** `research/oracles/v1/<corpus>/<oracle>.txt.gz` + `SHA256SUMS`, a new versioned path. Nothing under `profiling/` changes.

### 3.3 Dump format

The format is line-oriented text, one record per token, tab-separated:
- `a` = 0-based address index in the corpus file;
- `t` = token index;
- doubles are 16 hex digits of their IEEE-754 bits (`%016llx` of a `memcpy`'d `uint64_t`), never `%f`;
- strings are escaped (`\` → `\\`, TAB → `\t`, LF → `\n`).

| Oracle | Original code point | Record |
|---|---|---|
| **O1 tokens** | after tokenization in `address_parser_parse` | `a  N  num_tokens`, then per token `a  t  type  offset  len` |
| **O2 context** | end of `address_parser_context_fill` | `a  t  normalized  normalized_admin  dict(start,len,data)  comp(start,len,data)  postal(start,len,data)  prefix(start,len)  suffix(start,len)` |
| **O3k keys** | `crf_tagger_score`, per token, in emission order | `a  t  S|P  i  key`. `S` = `features`, `P` = `prev_tag_features`, `i` = position. **Ordered, duplicates kept.** |
| **O3i ids** | same loop, after `crf_get_feature_id` / `crf_get_state_trans_feature_id` | `a  t  S|P  i  id` or `-` (key absent from model, skipped by the original) |
| **O4 scores** | after token `t`'s last accumulation | `a  t  S  <L × hex64>` (state row), `a  t  T  <L·L × hex64>` (state-transition row) |
| **O5 labels** | existing `dump_outputs` | unchanged |

**Binding exactness condition for a rewritten feature stage: O3i with the `-` records dropped, as an ordered id sequence per token and list.**
- The original adds weight rows in exactly this order, and `cpi` keeps the order (`crf.c`, EXP_DEDUP_BATCH: "accumulated per token in the original order"). So equal sequences imply bit-equal score rows.
- O4 is checked anyway, to catch any downstream change.
- O3k is diagnostic only: a mismatch in O3i is explained by printing the original key beside it.

### 3.4 Comparison

Byte comparison decides: `cmp` and `shasum` of the candidate's dump against the oracle file. `tools/oracle_cmp.py` only *explains* a failure. It streams both gzip files and prints the first differing record with the address text and the original key, plus per-oracle mismatch counts.

A candidate H-build (§4) writes the same formats from the same code points. For the integer stages it writes O3i straight from its id stream, since no keys exist.

## 4. Hybrid builds (one stage rewritten, everything else original)

### 4.1 Flags

| Flag | Replaces | Must match |
|---|---|---|
| `EXP_RW_UNIT` | new per-token **unit id**, computed once in context fill. One interned id space for vocabulary words, dictionary phrases, component phrases, `UNKNOWN` and `UNKNOWN_NUMERIC`, so that equal ids ⇔ equal canonical strings (the strings `word_or_phrase_at_index` returns today) | O2 unchanged + self-check `unit_string(id) == original string` for every token |
| `EXP_RW_FEAT_<FAMILY>` | feature generation emits ids, not strings, one family at a time (`BIGRAM`, `WORD`, `NEIGHBOR`, `POSTCODE`, `DICT`, …) | O3i (ordered, misses dropped) |
| `EXP_RW_LOOKUP` | removes the string → id lookup for families already emitted as ids; other families keep the existing path | O3i, O4 |
| `EXP_RW_CF_<CARD>` | context-fill internals, only for cards that pass their probe (§7) | O2 |

### 4.2 Two build lines

- **H-builds: correctness only, never timed.** The original sources + `EXP_ORACLE` + exactly one `EXP_RW_*` stage; every other stage is original code. An H-build of family X emits ids for X only and leaves the other families as strings, so each family is accepted on its own. Each must match its oracles on all four oracle corpora before it is combined with anything.
- **R-builds: timed candidates.** `cpi` sources + the same rewritten files; back half unchanged.
  - The rewritten code is written against the original `address_parser_context_t` plus new side arrays, so it compiles into both lines.
  - Where `cpi` already changed that interface (FTT slots, 067 memo, 119 ids), an adapter is written and proven in an H-build first.
- **Combination order:** `UNIT` → `FEAT_<family>` (largest first: bigrams 9.6 keys/address, then word/neighbour, postcode, dictionary) → `LOOKUP` → R port → optional `CF_*`. A combined H-build must match all oracles before its R twin is built, and the R twin must pass end-to-end `verify.sh` (all sets, fuzz, unseen v4–v11 dumps).
- **Build traps** (HANDOFF §3): header shadowing (use accessors), `chmod +x`, unique variant names, PGO trained through the new paths (138's `build_cc_lapgo.sh`).

## 5. Milestones and kill rules (declared now, before any work)

Correctness gates every step. Speed is judged **only** at ⏱ milestones, with:
- the 121/103 protocol (original / `cpi` / candidate; golden-v1, novel, OSM; 5 rotating rounds; thread check; steady footprint);
- one never-used OSM set: v12 at M3, v13 at M5; **fetch v13 during M1**, and v14 before any later ⏱ milestone.

Intermediate slowdowns are logged, not fatal.

| # | Milestone | Done when | Kill / stop rule (declared) | Effort (UNVERIFIED) |
|---|---|---|---|---|
| M0 | Oracle build + dumps | identity control and negative controls pass; oracle files hashed | identity control fails → no rewrite work until fixed | 1 session |
| M1 | Probe, model + counters only, no parse-path change | per family: model key count, unit-id coverage of the 17.5 / 14.1 remaining string keys per address, table bytes; cost of computing unit ids measured with stage counters | **stop the track** if tables need > 50 MB extra peak (keeps 20 MB of the ≈70 MB headroom), or unit-id coverage of bigram parts < 90%, or > 5% of model bigram keys cannot be split unambiguously into two units | 1 session |
| M2 | H: `UNIT` + `FEAT_BIGRAM` exact | O2, O3i, O4 identical on all oracle corpora | not exact after 3 sessions → drop the bigram family, report why | 2–4 sessions |
| M3 ⏱ | R1 = cpi + UNIT + BIGRAM | end-to-end identical; stage timers; **121/103 protocol + unseen v12**. Development screens (screen.sh) are allowed for choosing among variants (§6) but never decide a kill | **stop the track** if, under 121/103, OSM cold features + lookups fall < 0.10 µs (split harness), or R1 is not ≥ +1.5% on both golden-v1 and OSM, or R1 is slower on v12. **Keep** as a population member at ≥ +1.5% with a timer mechanism | 1–2 sessions |
| M4 | H then R: remaining families + `LOOKUP` | all oracles; end-to-end | per family, as M2 | 3–6 sessions |
| M5 ⏱ | R2 validation | 121/103 protocol + unseen v13 (fetched during M1) | **stop and return to presentation polish** unless R2 is ≥ +5% vs cpi on both OSM and golden-v1 *and* not slower on v13 | 1 session |
| M6 | optional context-fill cards (C6–C8) | each card's own probe + kill rule, then H/R as above | per card | open |
| M7 | package + report | `final/<NNN>-rw`, report updated with verified numbers only | — | 1 session |

**Results so far (added 2026-10-04 after the runs):**
- **M0 passed** (experiment 142; oracles in `research/oracles/v1`).
- **M1** (experiment 143):
  - rules 2 and 3 pass (unit coverage 99.8–100%; ≤ 0.46% of keys not splittable unambiguously);
  - rule 1 is not triggered on measured evidence (122: −48 MB peak for the same pair move) but is unconfirmed for this design.
  - Added kill rule, declared before any build: **at M2/M3, stop if the candidate's peak footprint is > original − 20 MB (> 2,594 MB).**

- **Math check (144/145, `research/analysis/REWRITE_MATH_2026-10-04.md`):** the bigram card's exact gain is projected at 0–5% and the whole pipeline at ≈0–7.5% after 147's correction (UNVERIFIED; outer ceiling 6.9× golden-v1 / 6.7× OSM). 7.8× is not reachable through this track. M2 has been designed but not built; continuing is the user's call.

Overall time box: if M3 is not reached within **6 working sessions after approval**, stop and write up. The user may set a calendar date instead.

## 6. Population policy

- **Keep the top 2–3 variants alive**, each in its own experiment directory (`14x-rw-<variant>`). The expected variant axes are:
  - the pair-id store: open-addressed hash vs per-unit sorted neighbour arrays vs minimal perfect hash with stored keys (C1/C2);
  - the unit-id space: one interned space vs per-source ids plus an equality table (C3).
- **Selection discipline** (METHOD.md generalization 6):
  - choose on development only (stage timers + dev screen);
  - report validation, novel, OSM and unseen numbers for the chosen variant without re-choosing;
  - keep every variant's results;
  - only the chosen variant consumes an unseen set.
- **Dropping a variant.** A variant is dropped only by a kill rule, or when another variant beats it at a ⏱ milestone by ≥ 1.5% on both screening sets with a timer mechanism. The drop and its numbers go in the ledger.
- **Parent selection** is done by hand: the next milestone may branch from any live variant, not only the fastest. The choice and the reason are written into `REWRITE_PROGRESS.md`.

## 7. Idea cards

### 7.1 Card format (`research/plans/cards/Cnn-<slug>.md`, one page)

```
Card Cnn — <name>                Target (141 bucket): context fill | features | lookups | other | fixed
Source: <paper / system>         Status: proposed | probing | killed | adopted (exp NNN)
Mechanism: 3–5 lines; why it removes compute or dependent misses here.
Exactness obligation: what must be proven for byte-identity (ordered id sequence; no float op/order change;
  string equality ⇔ id equality; keys absent from the model; Unicode/normalization edge cases).
Memory: sizes from model counts against ≈70 MB peak headroom and the steady footprint.
Model-derived only: how every table/ranking comes from the model file or hardware facts (§2.5).
Probe: cheap counter or offline script, ≤ 1 session.
Kill rule: numeric thresholds, declared before the probe.
Expected gain: UNVERIFIED, with its arithmetic, in µs cold on dev and OSM.
```

### 7.2 First cards, ordered by the 141 bucket they attack

- **C1: Pair table keyed by unit ids** (features + lookups; dev 2.04 / OSM 1.29 µs cold).
  - *Mechanism.* At load, enumerate every `prev word+word` and `word+next word` key in the model, split it into its two unit strings, and map each to a unit id. Store `(unit_a, unit_b) → feature id` in an open-addressed table. At parse, a bigram feature is one probe with two integers: no string concatenation, no hashing of bytes.
  - *Exactness.* A split must be unambiguous (the separator may occur inside phrases, so check that every key splits one way). Unit-id equality must equal string equality. Pairs absent from the table must be exactly the pairs whose key is absent from the model.
  - *Memory.* Entries = model bigram keys (count unknown; M1 measures it). 122's integer pairs *lowered* peak by 48 MB, a hint that it fits.
  - *Probe/kill.* As in M1.
  - *Gain.* UNVERIFIED: 122 saved 0.03 µs in lookups but lost 0.08 µs in features to canonicalization. C1 only pays if C3 makes canonicalization free.
- **C2: Minimal perfect hashing for the remaining string families** (lookups). Source: the PTHash / CHD family of MPHFs.
  - *Mechanism.* Replace probing of the string dictionary with an MPHF over the model's keys, plus a stored key or full fingerprint check.
  - *Exactness.* An MPHF maps unknown keys to arbitrary slots, so the full key must be compared (fingerprints alone are not exact).
  - *Memory.* ≈3–4 bits/key + the existing key store.
  - *Kill.* If probes per lookup are already ≈1 (`fd_next` share in 124 ≈0.09 µs cold), the gain is ≤ 0.09 µs. Kill if the M1 counters show < 1.3 probes/lookup.
- **C3: One interned unit-id space, computed once per token in context fill** (features). This card is the enabler of C1.
  - *Mechanism.* Each token's canonical string (vocabulary word, phrase, or UNKNOWN / UNKNOWN_NUMERIC) gets one id at context fill, reusing the vocabulary hash lookup the FTT slot loop already does and 119's component-phrase key ids. word / prev word / next word / first word then become `table[family][unit]` reads.
  - *Exactness.* Strings shared between sources (a word that is also a phrase key) must map to one id. A load-time equality table, built by looking up every phrase key string in the vocabulary, provides this. The rare-word n-gram and sub-word paths (word frequency ≤ threshold) stay on strings.
  - *Memory.* 3.69M vocabulary + 4.01M component keys + 0.15M dictionary keys need per-family id arrays only where the model has the feature (sparse). A dense 7.85M × families × 4 B would be ≈ 30 MB per family, which is over budget for more than two families, so the arrays must be sparse.
  - *Kill.* > 50 MB total, or fewer than 90% of word-family keys covered.
- **C4: Table-driven feature emission ("feature program")** (features).
  - *Mechanism.* Precompute, per token-class signature, the ordered list of family slots to emit, replacing the branchy `address_parser_features` control flow.
  - *Exactness.* The emission order must equal the original's branch order; prove it with O3i.
  - *Gain.* Branch and compute only, small (UNVERIFIED). Kill if `features` warm time after C1 + C3 is < 0.3 µs.
- **C5: SIMD tokenizer / character classifier** (other: tokenize 0.18–0.21 µs cold). Source: simdjson-style classification.
  - *Exactness.* Token boundaries and types must be identical (O1). The tokenizer is a generated scanner (`scanner.c`) with complex Unicode rules, so the risk is high.
  - *Kill.* The ceiling is the whole tokenize stage (≤ 0.21 µs ≈ 4%). Probe the share of all-ASCII tokens first; kill if < 80%.
- **C6: Cache-friendly trie layout** (context fill; dev 1.72 / OSM 0.99 µs cold; trie walks ≈ 0.28 µs cold in 124).
  - *Mechanism.* Rebuild the component and postcode double-array tries in level-order or van-Emde-Boas order so a walk's first levels share lines.
  - *Exactness.* Same match set; prove it with O2.
  - *Kill.* Probe lines touched per walk; kill if < 30% reducible. This is golden-v1-leaning (HANDOFF §8.3).
- **C7: One multi-pattern pass for the three phrase searches** (context fill). Source: Aho–Corasick.
  - *Mechanism.* The dictionary, component and postcode searches each walk the same token sequence. One automaton could report all three.
  - *Exactness.* Each search's longest-match and overlap semantics must be reproduced exactly (O2), which is the high-risk part.
  - *Kill.* If the walks' cold time is dominated by first-node misses already served by FTT (109/125), the gain is < 0.1 µs.
- **C8: Score-row initialisation by write instead of memset + add** (fixed bucket, "rest": memset ≈ 0.17 µs warm OSM in 132).
  - *Exactness.* `0.0 + x` equals `x` bit-for-bit except when `x == -0.0`, where the sum is `+0.0`. So the probe must count `-0.0` weights in the model; if any exist they need special handling. Untouched classes must still read `+0.0`.
  - *Kill.* 128 (row masks) found no gain. Kill unless the probe shows the memset is still ≥ 0.1 µs cold on cpi.

Hashing tricks that collide (Vowpal-Wabbit-style feature hashing) are excluded, because collisions change output.

## 8. Outer-loop runner (design only; bash 3.2; built only after approval)

Files, all under `research/plans/runner/`:
- `REWRITE_PROGRESS.md` sits at `research/plans/REWRITE_PROGRESS.md` and is the single source of truth: current milestone, live variants, task list (`TODO` / `DOING <worker>` / `DONE <exp>`), the last session's summary, blockers, and a `STOP` line.
- `prompt.md`: the fixed session prompt. It says to read AGENTS.md, HANDOFF §2–3, METHOD.md, this plan and the progress file; do the next unblocked task; never run timing directly, only enqueue it; update the progress file, the ledger and the activity log before exiting.
- `outer.sh`: the session loop.
- `timing_worker.sh`: the single serial timing queue.
- `queue/timing/NNN.job`: one job per file, each a path to an experiment's `tools/run.sh`.
- `locks/`: `mkdir`-based locks (macOS has no `flock(1)`).
- `logs/session_NNN.json`.

`outer.sh` (sketch):

```bash
#!/bin/bash
# bash 3.2: no associative arrays, no GNU timeout.
R="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research"; D="$R/plans/runner"; PF="$R/plans/REWRITE_PROGRESS.md"
n=0; stall=0; prev=$(shasum "$PF" | cut -c1-40)
while ! grep -q '^STOP' "$PF" && [ ! -e "$D/STOP" ] && [ $n -lt ${MAX_SESSIONS:-20} ]; do
  n=$((n+1)); log="$D/logs/session_$(printf %03d $n).json"
  set -m   # job control: the session gets its own process group, so the watchdog can kill everything it spawned
  claude -p "$(cat "$D/prompt.md")" --output-format json --max-budget-usd ${BUDGET:-15} \
         --permission-mode acceptEdits --settings "$D/settings.json" \
         --allowedTools "Bash($D/build_job.sh:*)" "Bash($D/verify_job.sh:*)" "Bash($D/enqueue_timing.sh:*)" "Read" "Edit" "Write" > "$log" 2>&1 &
  pid=$!; set +m
  ( sleep ${SESSION_SECS:-14400}; kill -TERM -$pid 2>/dev/null; sleep 30; kill -KILL -$pid 2>/dev/null ) & dog=$!   # watchdog replaces `timeout`; kills the whole group
  wait $pid; rc=$?; kill $dog 2>/dev/null; kill -TERM -$pid 2>/dev/null   # also reap orphans left by a normal exit
  echo "$(date '+%Y-%m-%dT%H:%M:%S%z') session $n exit=$rc" >> "$D/logs/outer.log"
  for l in "$D"/locks/*; do [ -d "$l" ] && ! kill -0 "$(cat "$l/pid" 2>/dev/null)" 2>/dev/null && rm -rf "$l"; done   # stale-lock recovery
  cur=$(shasum "$PF" | cut -c1-40); if [ "$cur" = "$prev" ]; then stall=$((stall+1)); else stall=0; fi; prev=$cur
  [ $stall -ge 2 ] && { echo "STOP: two sessions without progress" >> "$PF"; break; }   # stalled → stop, a human looks
  sleep 30
done
```

Rules and conflicts the design resolves:

- **Settings / permissions.** `settings.json` denies Write/Edit under `profiling/**`, `research/final/**`, `research/experiments/000-baseline/**` and `research/oracles/**` (after M0). It also denies `git push`, network fetches other than the Overpass builder, and `kill` of non-project processes. Flags were checked against `claude -p --help` (Claude Code 2.1.289): `--output-format`, `--max-budget-usd`, `--permission-mode`, `--settings`, `--allowedTools` and `--disallowedTools` exist; `--max-turns` is **not** listed, so the watchdog and the budget are the limits.
- **Process hygiene.** A previous session's ASan binary ran for 6 days unnoticed (HANDOFF §3), so every session runs in its own process group. The watchdog and the post-exit cleanup kill the whole group (`kill -TERM -$pid`).
- **Locks.** Every `mkdir` lock directory holds a `pid` file with its owner's PID. A lock whose owner fails `kill -0` is stale and is removed. Locks are taken with `mkdir` and released with `rm -rf` from a `trap … EXIT`.
- **Bash permissions.** `--permission-mode acceptEdits` approves file edits only, not Bash. Headless sessions may therefore run only the three wrapper scripts listed in `--allowedTools`:
  - `build_job.sh` (build into an experiment directory);
  - `verify_job.sh` (`verify.sh` + oracle comparison);
  - `enqueue_timing.sh` (write a job file).
  The wrappers validate their arguments: paths inside `research/experiments/1[4-9][0-9]-*` only. Exact pattern syntax and behaviour must be proven with **one dry session** before the first real campaign; its log is kept.
- **Parallel workers: build and verify only.** A session may start up to **3** background build/verify jobs: 24 GB RAM, ≈2.6 GB per model-loading process, leaving ≈14 GB for the system and the user. Each job holds `locks/slot.K` (`mkdir` succeeds = slot taken; `rmdir` on exit, also from a `trap`).
- **The single timing queue, and its conflict.** `tools/env_gate.sh` fails while `clang`, `ld`, `dump_outputs`, `fuzz_pipeline` or `bench_ext` run, so parallel builds would starve the timing queue. The worker therefore runs in **phases**:
  1. take `locks/timing` (`mkdir`);
  2. while it is held, `outer.sh`'s sessions may not start new build/verify jobs, because each job checks for `locks/timing` before taking a slot;
  3. wait until all slots are free (drain);
  4. run the §3 hygiene checks (`top`, crashpad / icdd / leftover binaries; log them; never kill a user application), then `env_gate.sh`;
  5. run exactly one job serially;
  6. release the lock.
  Builds resume between jobs. Only the timing worker ever times anything.
- **Stopping.** The runner stops on any of: a `STOP` line, a `STOP` file, the session cap, a ≥ 2-session stall, or a gate `ABORT`. A session that claims "exhausted" is not trusted. The outer loop simply starts a fresh session against the progress file; a human looks only after two non-progressing sessions.
- **Auditability.** Each session's JSON log, cost and exit code is kept. Each experiment keeps the normal `raw/` records. The progress file is append-only for the `DONE` lines.

## 9. Cost, risk and the trade-off

- **Engineering cost (UNVERIFIED):**
  - M0–M3 is ≈ 5–8 sessions;
  - through M5 is ≈ 10–16 sessions;
  - REACHABILITY_2026-10-02 estimated a front-half rewrite at weeks.
  The headless runner costs API spend per session: this interactive session alone reached ≈ $34 by the time 141 finished. A `--max-budget-usd` of 15 × 20 sessions caps one runner campaign at $300. The user should set the real cap.
- **Expected payoff (UNVERIFIED):** between a few percent (the extrapolation from 119) and ≈ +16–28% over cpi (the assumption that 40–60% of features + lookups is removed), i.e. up to ≈ 6.7–7.3× OSM and ≈ 7.2–7.8× golden-v1 development (§2). **7.8× on OSM-style data is unlikely from this track alone.** It needs 42–47% of the whole front half removed (74–80% of features + lookups), against an identity-fixed floor of ≈0.8 µs.
- **Exactness risk.** It is concentrated in C3: unit-id equality ⇔ string equality across vocabulary, phrase and unknown sources, plus the rare-word paths. The oracles turn this into localized, checkable failures but do not remove it.
- **Memory risk.** Pair and unit tables vs ≈70 MB headroom. M1 decides with model counts before any parse-path code is written.
- **Presentation risk.** The professor-facing deliverable today is a validated, packaged 6.08× / 5.93× / 5.78× with unseen confirmations. Open items that do not depend on this track:
  - packaging (aarch64 guard for 091, hard-coded paths, the one-command benchmark);
  - load time (12 s vs 3 s);
  - an ablation and confidence intervals.
  An unfinished rewrite adds nothing to the presentation, while the polish items directly reduce the risk that the professor's own measurement disagrees.
- **Recommendation.** Do the presentation polish first (packaging and the one-command benchmark at minimum). Then run M0 + M1 (≈2 sessions), which are cheap and decisive: M1's memory and coverage numbers either kill the track early or justify M2–M5. Do not start the runner before M1 has passed.

## Status (2026-10-04T22:17:18-0700): track stopped

- M0 passed (142); M1 passed rules 2/3 (143); math check (144/145); M2 passed (146).
- 147/148 diagnosed why 122's integer emission is not cheaper.
- M3's R-build (149) failed declared rule 4: no variant ≥ +1.5% on both screening sets.
- The track stops here by its own rules. M3 validation, M4–M7 and the outer-loop runner were not started.
- golden-v12 remains unused. golden-v13 was built: 9,716 addresses, 58 of 65 cities fetched (7 failed with Overpass 504s), unused.
