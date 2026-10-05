# libpostal address parser: 3.5× target → ~6× achieved

**Goal (from Prof. Jiapeng):** make libpostal's address parser **≥ 3.5× faster on one thread**, with **no more memory**.
We also required **byte-identical output** to the original on every input.

## Result

| | |
|---|---|
| Speedup | **5.84×** overall (each data source weighted equally): **6.15×** golden-v1, **5.99×** novel, **5.29–5.75×** on 9 OpenStreetMap sets |
| Output | byte-identical on all 11 sets (110,551 addresses), 2 frozen references and 4,000,000 fuzzed inputs |
| Threads | 1 |
| Memory | peak 2.543 GB vs 2.614 GB (−71 MB); steady 1,927 MB vs 2,492 MB (−565 MB) |
| Load time | 11.6 s vs 2.75 s. This is a one-time setup cost, excluded from the per-address speed. |

Full run, 2026-10-04 22:36–23:05 (`research/suite/results/20261004-223647/report.md`). Speedup = original µs ÷ new µs in the same round; 5 rounds × 20 passes; 95% CI is a t-interval over rounds.

| Dataset | Source | Addresses | Original µs | New µs | **Speedup** | 95% CI |
|---|---|---:|---:|---:|---:|---|
| golden-v1 | Senzing | 12,863 | 32.87 | 5.13 | **6.15×** | 5.97–6.34 |
| novel | Senzing-style (new strings) | 12,863 | 31.59 | 5.34 | **5.99×** | 5.69–6.30 |
| osm-v3 | OpenStreetMap, 49 countries | 11,405 | 18.72 | 3.33 | **5.56×** | 5.36–5.76 |
| unseen v4–v11 (8 sets) | OpenStreetMap, new cities each | 73,420 | 17.3–18.5 | 3.08–3.43 | **5.29–5.75×** | each CI within 4.80–6.26 |

An earlier validation on a quieter machine (experiment 121) gave 6.08× / 5.93× / 5.78× for the first three rows.

## Datasets

- **13 sets, 128,540 addresses, 2 independent sources.**
  - **golden-v1** (Senzing open data): split into development (10,308) and validation (2,555).
  - **novel**: new strings in the same style, with reordered components, casing and typos.
  - **11 OpenStreetMap sets**: v3–v13, with disjoint cities and no duplicate strings.
- **v12 (8,273) and v13 (9,716) are still unseen**: never run, reserved for a final check on new data.
- **Tuning used golden-v1 development only.** All tables derive from the model file, never from the test data. Compiler PGO also trained on golden-v1 development; experiment 031 shows the training set changes the speed by ≤1.7%.

## How it was done

The model and the arithmetic are unchanged. The work removes overhead around them; every change is exact.

1. **Faster lookups.** The feature tries are replaced by hash dictionaries, with integer keys for word features and ids precomputed at load for 4M place names (`119`).
2. **Less string work.** Name-byte keys, joins with known lengths, letter and digit fast paths, one-pass paired normalization, and a per-address phrase cache.
3. **Model-derived tables.** A first-token table for the city search, prefix/suffix flags, and a losslessly compressed postcode graph (frees 323 MB).
4. **Decoding.** A Viterbi specialised to the 20 labels, NEON, max-only forward pass.
5. **Memory latency.** Prefetches issued one stage ahead, from data already in hand.
6. **Build.** Whole-library PGO + LTO + mimalloc.

## Rerun (one command)

```sh
benchmark/run.sh            # ~1 h: fetch libpostal @25099c50 + model v1.0.0, build original and optimized, verify identical output, time, report
benchmark/run.sh --quick    # ~15 min smoke test (rough numbers)
```

The script:
1. downloads the model (~1 GB) and checks it against `benchmark/model_sha256.txt`;
2. builds the original with libpostal's own autotools, and the optimized version from `research/final/119-cpi`;
3. runs `research/suite` and writes the report to `research/suite/results/<run>/report.md`.

- **Requirements:** macOS on Apple Silicon, Xcode command-line tools, Homebrew (`autoconf automake libtool mimalloc`; `--deps` installs them), and Python 3.
- **Options:**
  - `--model-dir DIR` reuses a local model;
  - `--no-gate` skips the AC-power and thermal check;
  - `--include-reserved` adds unseen v12/v13 (uses them up).
- **Not in git:** binaries, build artifacts and bulky regenerable dumps (24 GB). They are listed with SHA-256 in `research/EXCLUDED_FILES.sha256`.

## What we tried (150 recorded experiments, failures included)

| Family | Outcome |
|---|---|
| Precompute from the model, look up at parse | **biggest wins** (dictionaries, int keys, first-token table, place-name ids) |
| Exact compute cuts (Viterbi, normalization) | wins, largest on OpenStreetMap data |
| Prefetching | wins only when issued a whole stage early; 10+ variants slower |
| Weight compression, hot/cold tiers, dense rows | rejected: the weights are 65–93% unique doubles, and hot rows are already cached |
| Cross-address tricks (lookahead, coroutine interleaving) | exact but 3–10% slower |
| Integer-feature rewrite of the front half (141–149) | exact at every stage, but **not faster** (0.985–1.01×) |

## Why ~6× is the ceiling without changing the rules

1. **One line explains every dataset.** Over 499 country groups, **new µs ≈ 0.53 + 0.149 × original µs** (R² = 0.92).
   - About 15% of the original's work remains, so the cap is 6.7× on very expensive addresses.
   - A ~0.53 µs per-address floor pulls short addresses down. That is why OSM (18 µs) gets 5.6× and golden-v1 (33 µs) gets 6.0×.
2. **Part of the remaining time is fixed by byte-identity.** Accumulating the scores and Viterbi decoding take 0.8–0.9 µs per address. Changing them changes the floating-point order, and so the output.
3. **The warm bound** (all memory waits removed) is 12.9× golden-v1 and 8.6× OSM. But 15 later memory-side experiments (prefetch, layout, tiers) were rejected, each gaining a few percent at most or losing time.
4. **The largest remaining exact lever was measured.** Deleting the most expensive feature family outright saves only 7–9%. The exact integer version of it saves ~0%, because its bookkeeping costs as much as the string work it replaces.
   - Even if every remaining string path were free, the ceiling is ≈6.9× golden-v1 and ≈6.7× OSM.

**Going further needs a rule change:** approximate output (feature pruning or float32 weights), a smaller or retrained model, multithreading, or caching across addresses.

Derivation: `research/analysis/REWRITE_MATH_2026-10-04.md`.

## Effort (honest)

- **Span:** 2026-09-27 to 2026-10-05, 9 calendar days.
- **Sessions:** 11 Claude Code sessions, 339 user turns (this count includes some automated notifications).
- **Time:** ~59 h of active session time, counting no gap longer than 30 min. Much of it is unattended benchmark runs, not hands-on time.
- **Tooling:** the work was AI-assisted throughout (Claude Code). Every number traces to raw logs.

## Limitations

- **Hardware:** measured on one Apple M4 Pro laptop.
- **Sources:** the OpenStreetMap sets share one generator, so they test new cities, not a new kind of data.
- **Platform:** the NEON Viterbi backtrack is not yet guarded for non-Apple-Silicon builds (open packaging item).

## Where everything is

- **Experiment ledger:** `research/experiments/README.md` (each experiment has commands, raw logs and hashes).
- **Method and rules:** `research/METHOD.md`.
- **Chronology:** `research/ACTIVITY_LOG.md`.
- **Previous long-form report:** `research/archive/README_full_2026-10-04.md`.
