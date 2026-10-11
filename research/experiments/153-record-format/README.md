# Experiment 153 — PLAN A2: memory-feasible compiled-record format, built from the whole model (no data input, no library change)

Date/time/timezone: created 2026-10-05 (`raw/created.txt`), America/Los_Angeles. Run 15:20:02 → 15:20:12 (`raw/run.log`).
Status: **A2 passed** (exactness by full check; memory by accounting, with the peak projected and UNVERIFIED until a build loads it).

## Hypothesis, change and kill rule (from `151/PLAN.md`, declared before running)

Hypothesis: every model feature row can be stored in "compiled lexicon records" keyed by strings the parser already holds (word, phrase or
word pair), and the records fit in the memory of the structures they replace.

Kill rule: any of the following stops the track:
- any model key is unmapped, or reads back differently;
- arena + index exceed the structures they replace;
- the projected peak is above original − 20 MB (2.594 GB).

Change: none to the library. `tools/a2_records.c` links the preserved ORIGINAL `libpostal.a` (`crf_load` for rows) and reads 143's key dumps. The
dumps were regenerated with 143's `keydump` into the scratchpad. **Their SHA-256 values equal 143's** (`raw/keys.sha256` vs `143/raw/keys.sha256`).

**Grouping rule.** It is reproducible at runtime with no key building; header of `tools/a2_records.c`.

| Record | Key | Families it holds |
|---|---|---|
| unit | the word, phrase or n-gram string | 10 unit families plus `commonly *`, the 2 prev-tag families, and `(unambiguous) phrase type+phrase` with the type in the slot |
| pair | the string `a\|b` | both bigram families |
| own | the full key | the 2,298 remaining keys |

The vocabulary frequency sits in the unit record.

## Results (`raw/a2.stdout`)

| | value |
|---|---|
| model entries | 11,470,623 state + 2,086,474 transition keys + 3,694,560 vocabulary words |
| records | 10,025,721: 5,893,398 unit (2,687,878 of them vocabulary-only), 4,130,025 pair, 2,298 own |
| slot families | 58; duplicate (record, kind, family) slots: 0 |
| **verification** | **13,557,097 rows bit-identical (labels and value bits), 0 mismatched, 0 unmapped; 3,694,560 vocabulary frequencies equal** |
| index probes per lookup | 1.246 mean |
| arena f64 / f32 | 795.9 / 577.9 MB (record keys 160.1 MB) |
| index (load 0.30, power of two) | 268.4 MB |
| **total f64** | **1,064.4 MB** (f32: 846.4 MB) |

### What it replaces in `cpi` (each figure has a source)

| `cpi` structure | MB | source |
|---|---:|---|
| state + transition feature dictionaries (string + integer keys; keys only, no rows) | 529.5 | 143 `raw/cpi_dict_verbose.stderr`; `feature_dict.h:3` slot layout |
| vocabulary dictionary | 115.3 | same |
| packed weight records {f64, u16}, 10 B × (33,799,350 + 13,369,573) | 471.7 | `119-cpi/sources/crf.c:11` (packed struct); nonzero counts from 151 |
| row index (`indptr`) kept by `crf_sparse_read_packed` | 54.2 | `crf.c:26`; 4 B × (11,470,624 + 2,086,475) |
| **total replaced** | **1,170.7** | |

**Kill-rule check.**
- **Size.** Records at 1,064.4 MB ≤ 1,170.7 MB replaced, a net change of **≈ −106 MB**.
- **Peak.** The projected peak is 2.543 − 1.171 + 1.064 ≈ **2.44 GB**, under the 2.594 GB limit. This is UNVERIFIED: it assumes the arena is produced offline and loaded without a second transient copy.
- **The f32 variant is not adopted.** Its fallback needs the exact doubles. Storing both would add f32 + f64 and exceed the budget. PROOF.md §6 also makes f64 unnecessary to give up: in emission order, f64 records are exact with no certificate.

## Correction after experiment 155 (2026-10-05)

The arena 155 actually timed has per-slot 8-byte headers that carry the transition-row mask K3 needs. It is **887.8 MB** (155 `raw/budget/rr_*.stdout`); with the 268.4 MB index, the total is **≈1,156 MB**.
- Against the 1,170.7 MB it replaces, the net is only **≈ −15 MB**, and the projected peak is 2.543 − 1.171 + 1.156 ≈ **2.53 GB**. That still passes the 2.594 GB line, with little headroom.
- **Available, not claimed:** an index at load 0.6 with a non-power-of-two size (≈ −134 MB).
- The −106 MB figure above describes the 4-byte-header format **without** masks.

## Interpretation and limitations

- **The format is exact.** Every model row survives, bit for bit, and lookup by the runtime's own strings finds it.
- **The memory fits, without much headroom to spare.** Two savings are available and not needed now:
  - an index at load 0.6 with a non-power-of-two size (≈ −134 MB);
  - unit ids in place of full pair strings (≈ −100 MB of keys, UNVERIFIED).
- **Load time is not addressed.** This build took ≈10 s in a tool. A5 must ship the arena as an offline file derived from the model.
- **The grouping is not yet proven to give every lookup the parser makes.** It covers every key that *exists in the model*. A4 must also produce exactly the original misses: a key absent from the model is simply a missing slot.

## Decision

A2 passed. The record format is f64 records in emission order, with no certificate needed. Ledger and activity log updated.
