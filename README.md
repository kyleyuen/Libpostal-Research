# Libpostal address parser optimization

**Result.** On one Apple Silicon Mac, the optimized build of libpostal's default address parser parsed addresses 5.5–6.1× faster than the original once loaded (warm parsing), on one thread, with byte-identical output on all 130,341 tested rows (14 datasets) and lower peak memory in all 42 paired runs. The original target was 3.5×. The cost is startup: loading takes about 6.9 s longer (9.64 s vs 2.79 s median), so the optimized build only finishes sooner overall once one process parses roughly 260,000–550,000 addresses (see [Startup cost](#startup-cost)). Parity of the public API beyond the default parser is unverified: a public feature-print API probe aborted in the earlier 119 build, and 203 ran no full API suite.

The current candidate is called **203** (its experiment number). It builds on the **CPI** (component phrase IDs) design from experiment 119 and adds a streamed postcode-graph loader, so the full graph index and its compressed form are never resident at the same time during setup.

## Glossary

| Term | Meaning |
| --- | --- |
| original | Unmodified libpostal at pinned commit `25099c50`; preserved binaries are in `research/experiments/000-baseline/artifacts/`. |
| CPI | "Component phrase IDs", the experiment-119 parser design that the candidate builds on. |
| 203 / candidate / promoted | The current optimized build (experiment 203). "Promoted" means it was accepted as the current candidate after the gates in [METHOD.md](research/METHOD.md), for the default-parser scope. |
| warm parsing | Median time per address over passes 2..N within one process, after the model is loaded. Startup is reported separately. |
| PGO | Profile-guided optimization: the compiler is trained by running the parser on `golden-v1/development.txt`, then rebuilds using that profile. |
| golden-v1 | 12,863 Senzing-sourced test addresses, split into development (10,308) and validation (2,555) files that share no strings. |
| novel | A second Senzing-derived set (`profiling/golden-v2-novel`). The "Senzing" cohort is golden-v1 plus novel. |
| OSM v3–v13, v14 | Address sets generated from OpenStreetMap for different city samples. v4–v14 were built as fresh "unseen" re-measurement sets and were not used for PGO. They share one generator, so they test new cities, not a new source, and METHOD.md records that the earlier sets were later observed during development, so none is a never-seen test set. |
| mimalloc | Microsoft's open-source memory allocator, linked into the candidate in place of the system allocator. |

## Get the repository

This GitHub repository is private. You need collaborator access before cloning; credentials are not stored here.

```sh
git clone https://github.com/kyleyuen/Libpostal-Research.git
cd Libpostal-Research
```

**Quick verify (under a second; no build, no download).** From the repository root run `python3 research/tools/verify_203_clone_evidence.py`. It prints `PASS` after checking the promoted package hashes, all 14 output pairs, the 84 timing captures, the 42 paired memory comparisons, and the live thread samples. It does not validate the external model files or rerun parsing.

**Rebuild (about 11 minutes of building plus a ~760 MB download; ~2.8 GB disk).** Run `./benchmark/run.sh --setup-only`; see [SETUP.md](SETUP.md). A fresh-clone audit on 2026-10-10 took 38 minutes in total, 27 of them downloading the model.

For a guided review, follow [REVIEW_GUIDE.md](REVIEW_GUIDE.md). The current candidate and tested scope are summarized in the [promoted 203 build record](research/final/203-cpi-memory/README.md); the [203 experiment record](research/experiments/203-cpi-memory-regression/README.md) preserves the diagnosis and raw validation paths. The [202 record](research/experiments/202-cpi-final-validation/FINAL_VALIDATION_REPORT.md) remains the historical result for the earlier CPI build.

## What this project changed vs upstream libpostal

Upstream [libpostal](https://github.com/openvenues/libpostal) (MIT License, © 2015 openvenues; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)) is not vendored. Setup fetches it at commit `25099c506612b34b23b1bfe286ca6321fcf06f35`. This project's own work is:

- **Parser source changes** in [research/final/203-cpi-memory/sources/](research/final/203-cpi-memory/sources/). Twelve upstream files are modified, +2,627 / −54 lines against the pinned commit. The largest changes are `address_parser.c` (+927), `crf.c` (+896), `trie_search.c` (+228) and `features.c` (+195). There are also three new headers (`feature_dict.h`, `stream_postcode_graph.h`, `fa_nb.h`; 654 lines), and `tokens.c` is carried unchanged. The changes are mostly guarded by the `EXP_*` compile switches listed in [BUILD_SWITCHES.txt](research/final/203-cpi-memory/BUILD_SWITCHES.txt).
- **Build configuration**: `-O2`, full LTO, PGO, an `-O1` tokenizer, and mimalloc ([BUILD.md](research/final/203-cpi-memory/BUILD.md)).
- **Measurement and verification tooling**: the suite runner ([research/suite/](research/suite/)), the benchmark, dump and fuzz harnesses and verifiers in [research/tools/](research/tools/), and [benchmark/run.sh](benchmark/run.sh).
- **Datasets and references**: the versioned test sets and frozen original outputs under [profiling/](profiling/).
- **Experiment history**: 204 numbered experiments (000–203), including rejected ones, in the [experiment ledger](research/experiments/README.md).

The speedup therefore combines source changes, compiler settings (LTO and PGO) and the allocator. It is not a purely algorithmic ratio.

## Methodology and hardware/software

Across 14 datasets (130,341 rows), candidate dump JSONLs were byte-compared with cached original JSONLs. Thirteen caches predated 203; the v14 original cache came from 202’s fresh original run. Input and artifact hashes were checked, as were frozen golden-v1 and novel outputs; the negative control detected a deliberate one-byte mutation. Two 2-million-case candidate fuzz jobs matched preserved original digest/status references; original fuzz was not rerun. These checks do not cover every possible input.

Timing used three rounds and at least 10 seconds of parsing per capture: 200 passes for Senzing, 420 for OSM, and 2,300 for v14. Runs were gated on AC power and captured load telemetry; load varied. Parsing stayed single-threaded in representative live samples on golden-v1, OSM v3, and v14. The [measurement method](research/METHOD.md) describes gates and calculations.

The promoted 203 build uses Apple Clang 17, `-O2`, full LTO, an `-O1` tokenizer, mimalloc 3, and the whole-library PGO profile trained on golden-v1 development data. That profile is committed as [pgo/dev.profdata](research/final/203-cpi-memory/pgo/dev.profdata). Model data is libpostal v1.0.0, which setup checks against recorded SHA-256 hashes.

The machine is described in separate records, not in one per-run specification. The 202 capture reports 28,160 MB total memory. A 2026-10-05 audit records Mac16,8, macOS 26.5.2, Apple Clang 17.0.0, and Python 3.14.4. Older golden-v1 results identify an Apple M4 Pro. All timing comes from this one machine; other Apple Silicon generations are untested. See [SETUP.md](SETUP.md) and the [203 build record](research/final/203-cpi-memory/BUILD.md) for provenance.

## Data and file formats

`profiling/golden-v1/all.txt` contains 12,863 unique addresses, one per line; five duplicate strings were removed from 12,868 source records. Development and validation splits are in separate text files. The source CSV at `profiling/golden-v1/raw/senzing-v1.2.0.csv` has 21 fields: `record_id`, `full_address`, `house`, `house_number`, `road`, `po_box`, `unit`, `level`, `staircase`, `suburb`, `city_district`, `city`, `state_district`, `state`, `postcode`, `country`, `world_region`, `category`, `near`, `country_code`, and `source`.

The paired benchmark TSV has no header and each row is a lowercase two-letter country code, tab, and address; the matching text file has one address per line in the same order. Annotated records JSONL uses `id`, `address`, `country_codes`, `source_rows`, and `split`. Frozen `baseline-outputs.jsonl` uses `id` and ordered `components` label/component pairs; live dump JSONL uses `address` and `components`. Parser outputs are comparison references, not ground truth. Empty CSV cells remain empty strings and their real-world meaning is unspecified. Cities with no usable OSM addresses add no row; missing or unrun measurements are unavailable, not zero.

## Verified result

| Measure | 203 vs original |
| --- | --- |
| Correctness | Byte-identical output on 14 datasets, 130,341 rows; the negative control detected a one-byte change. |
| Warm parsing speed | 6.11× golden-v1, 5.83× novel, 5.55× geometric mean over the 11 OSM sets, 5.72× v14. The cohorts were timed separately and are not pooled. |
| Startup | Slower: 9.64 s vs 2.79 s median (+6.87 s). |
| Peak memory | Lower peak physical footprint and lower peak RSS in all 42 paired runs. |
| Threads | One parser thread in all 36 live samples. |

These checks passed on the 130,341 tested rows. Finite checks do not prove behavior for every possible input.

The headline ratios are geometric means of the three per-round warm ratios, which is how the suite reports them. [METHOD.md](research/METHOD.md) defines speedup as median over median. Computed that way from the same captures, the values are 6.03× for golden-v1, 5.93× for novel and 5.72× for v14, and the 11 OSM sets range from 5.48× to 5.70× ([analysis output](research/analysis/203-startup-breakeven/output.txt)). Every paired per-round ratio exceeded 1.

All 42 same-dataset, same-round pairs had lower peak physical footprint (reductions of 502,775,976–517,472,400 bytes) and lower peak RSS. Representative steady footprint was lower on all three sampled datasets, with one sample each.

The 202 check of the older 119 CPI build passed its global maximum comparison by 70,500,184 bytes, but all six Senzing paired peak-footprint comparisons regressed: the golden-v1 maximum was +131,236,152 bytes, and the Senzing subgroup maximum was +58,982,640 bytes (novel, round 2). The streamed 203 candidate removes that measured per-dataset issue across its 42 pairs (84 captures). Timing ran on AC power, but system load varied. The known public feature-print API abort remains a limitation of the earlier CPI build, and 203 did not run a full API suite. See the [203 candidate record](research/final/203-cpi-memory/README.md), [output audit](research/audits/203-output-independent-audit-v1.md), [timing audit](research/audits/203-timing-final-independent-audit-v1.md), and [steady-memory audit](research/audits/203-steady-memory-independent-audit-v1.md) for scope and raw evidence.

## Startup cost

The warm ratios exclude library and model loading. The candidate builds its compressed in-memory structures at load time, so it starts more slowly. In the same 84 recorded captures (42 same-round pairs), setup took 9.64 s for the candidate and 2.79 s for the original (medians), 6.87 s longer.

Break-even is the number of addresses one process must parse before the candidate's total time (setup plus parsing) drops below the original's. It is computed from warm per-address times:

| Dataset group | Break-even addresses (range over rounds) |
| --- | --- |
| golden-v1 | 262,483 – 271,644 |
| novel | 258,626 – 290,674 |
| OSM v3–v13 | 442,473 – 501,785 |
| v14 | 545,695 – 553,929 |

Across all 42 pairs the median is 477,308 addresses. Put another way, the candidate wins once the original would have spent 8.2–9.4 seconds parsing.

The optimized build therefore suits long-running services and large batch jobs. For one-shot command-line use on a few thousand addresses, the original finishes first. This estimate ignores the slower first (cold) pass, so it describes the warm state. The script and per-pair output are in [research/analysis/203-startup-breakeven/](research/analysis/203-startup-breakeven/).

## Main folders

| Path | Purpose |
| --- | --- |
| `profiling/` | Versioned datasets, annotations, and frozen output references. |
| `research/final/203-cpi-memory/` | Current streamed CPI source, tested binaries, PGO profile, and build records. |
| `research/final/119-cpi/` | Preserved pre-streaming CPI source and build records. |
| `research/experiments/` | One directory per numbered experiment (000–203), indexed by the [ledger](research/experiments/README.md). |
| `research/experiments/000-baseline/artifacts/` | Preserved original binaries. |
| `research/suite/` and `research/tools/` | Validation runner, configurations, results, and helper scripts. |
| `research/audits/`, `research/analysis/` | Independent audits of the 203 result and cross-experiment analyses. |
| `benchmark/` | One-command setup and benchmark script. |

The [research index](research/README.md) explains the remaining files in `research/`, including dated working notes that are kept for the audit trail but are not required reading. See the [measurement method](research/METHOD.md) and [experiment ledger](research/experiments/README.md). Investigator, contact, and funding details are not recorded in the repository materials.
