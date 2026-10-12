# Libpostal address parser optimization

**Result.** On one Apple Silicon Mac, the optimized build of libpostal's default address parser is 5.5–6.1× faster than the original once it is loaded (warm parsing). That is on one thread. The original target was 3.5×. Output is byte-identical on all 130,341 tested rows (14 datasets). Peak memory is lower in all 42 paired runs.

The cost is startup. Loading takes about 6.9 s longer (9.64 s vs 2.79 s median). So the optimized build only finishes sooner overall once one process parses roughly 260,000–550,000 addresses (see [Startup cost](#startup-cost)).

I also compared the public API on fresh builds. Every family except one gave byte-identical results ([experiment 204](research/experiments/204-public-api-parity/README.md)):

- Expansion, language classification, deduplication, normalization and parsing with options: compared on 4,356 addresses.
- Repeated setup/teardown: compared on 200 addresses.
- The toponym check: compared on 189 address pairs, until upstream libpostal crashed identically in both builds.

The one exception is the feature-printing debugging switch. The optimized build refuses it by design. These inputs are finite.

A separate, much larger test used 7,266,489 real addresses from 88 countries, built from OpenAddresses and OpenStreetMap. The optimized build was 4.96× faster warm, and every country fell between 3.83× and 6.17×. Output was byte-identical, peak memory was lower, and it ran on one thread ([experiment 206](research/experiments/206-v15-88-country-benchmark/README.md)).

The current candidate is called **203**, which is its experiment number. It builds on the **CPI** (component phrase IDs) design from experiment 119. It adds a streamed postcode-graph loader, so the full graph index and its compressed form are never in memory at the same time during setup.

## Glossary

| Term | Meaning |
| --- | --- |
| original | Unmodified libpostal at pinned commit `25099c50`. The preserved binaries are in `research/experiments/000-baseline/artifacts/`. |
| CPI | "Component phrase IDs". This is the experiment-119 parser design that the candidate builds on. |
| 203 / candidate / promoted | The current optimized build (experiment 203). "Promoted" means it passed the gates in [METHOD.md](research/METHOD.md) and was accepted as the current candidate, for the default-parser scope. |
| warm parsing | Median time per address over passes 2..N within one process, after the model is loaded. Startup is reported separately. |
| PGO | Profile-guided optimization. We run the parser on `golden-v1/development.txt` to train the compiler. Then it rebuilds using that profile. |
| golden-v1 | 12,863 Senzing-sourced test addresses. They are split into development (10,308) and validation (2,555) files that share no strings. |
| novel | A second Senzing-derived set (`profiling/golden-v2-novel`). The "Senzing" cohort is golden-v1 plus novel. |
| OSM v3–v13, v14 | Address sets generated from OpenStreetMap for different city samples. v4–v14 were built as fresh "unseen" re-measurement sets and were not used for PGO. They share one generator, so they test new cities, not a new source. METHOD.md records that the earlier sets were later observed during development. So none is a never-seen test set. |
| mimalloc | Microsoft's open-source memory allocator. The candidate links it in place of the system allocator. |

## Get the repository

The repository is private. You need collaborator access to clone it. No credentials are stored here.

```sh
git clone https://github.com/kyleyuen/Libpostal-Research.git
cd Libpostal-Research
```

**To test it yourself, including on your own address data, follow [TESTING.md](TESTING.md).**

**Quick verify (under a second; no build, no download).** From the repo root, run `python3 research/tools/verify_203_clone_evidence.py`. It prints `PASS` after checking the promoted package hashes, all 14 output pairs, the 84 timing captures, the 42 paired memory comparisons, and the live thread samples. It does not check the external model files or rerun parsing.

**Rebuild (about 11 minutes of building plus a ~760 MB download; ~2.8 GB disk).** Run `./benchmark/run.sh --setup-only`. See [SETUP.md](SETUP.md). A fresh-clone audit on 2026-10-10 took 38 minutes in total, 27 of them downloading the model.

For a guided review, follow [REVIEW_GUIDE.md](REVIEW_GUIDE.md). The [promoted 203 build record](research/final/203-cpi-memory/README.md) covers the current candidate and tested scope. The [203 experiment record](research/experiments/203-cpi-memory-regression/README.md) keeps the diagnosis and raw validation paths. The [202 record](research/experiments/202-cpi-final-validation/FINAL_VALIDATION_REPORT.md) is the historical result for the earlier CPI build.

## What this project changed vs upstream libpostal

Upstream [libpostal](https://github.com/openvenues/libpostal) (MIT License, © 2015 openvenues; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)) is not vendored. Setup fetches it at commit `25099c506612b34b23b1bfe286ca6321fcf06f35`. This project's own work is:

- **Parser source changes** are in [research/final/203-cpi-memory/sources/](research/final/203-cpi-memory/sources/). Twelve upstream files are modified: +2,627 / −54 lines against the pinned commit. The biggest are `address_parser.c` (+927), `crf.c` (+896), `trie_search.c` (+228) and `features.c` (+195). There are also three new headers (`feature_dict.h`, `stream_postcode_graph.h`, `fa_nb.h`; 654 lines). `tokens.c` is carried unchanged. Most changes sit behind the `EXP_*` compile switches in [BUILD_SWITCHES.txt](research/final/203-cpi-memory/BUILD_SWITCHES.txt).
- **Build configuration** is `-O2`, full LTO, PGO, an `-O1` tokenizer, and mimalloc ([BUILD.md](research/final/203-cpi-memory/BUILD.md)).
- **Measurement and verification tooling** is the suite runner ([research/suite/](research/suite/)), the benchmark, dump and fuzz harnesses and verifiers in [research/tools/](research/tools/), and [benchmark/run.sh](benchmark/run.sh).
- **Datasets and references** are the versioned test sets and frozen original outputs under [profiling/](profiling/).
- **Experiment history** is 207 numbered experiments (000–206), including rejected ones, in the [experiment ledger](research/experiments/README.md).

So the speedup comes from source changes, compiler settings (LTO and PGO) and the allocator together. It is not a purely algorithmic ratio.

## Methodology and hardware/software

I tested 14 datasets (130,341 rows). Each optimized output was compared byte for byte with the original's saved output, and input and file hashes were checked. A negative control (a deliberate one-byte change) was caught. Two fuzz runs of 2 million generated inputs each matched the original's saved results; the original's fuzz run was not repeated. These checks don't cover every possible input.

Timing used three rounds, with at least 10 seconds of parsing per capture: 200 passes for Senzing, 420 for OSM, and 2,300 for v14. Runs were gated on AC power, and I captured load telemetry. Load varied. Parsing stayed single-threaded in representative live samples on golden-v1, OSM v3, and v14. The [measurement method](research/METHOD.md) has the gates and calculations.

The promoted 203 build uses Apple Clang 17, `-O2`, full LTO, an `-O1` tokenizer, mimalloc 3, and the whole-library PGO profile trained on golden-v1 development data. The profile is committed as [pgo/dev.profdata](research/final/203-cpi-memory/pgo/dev.profdata). Model data is libpostal v1.0.0. Setup checks it against recorded SHA-256 hashes.

Machine: Apple M4 Pro (Mac16,8), 28,160 MB memory, macOS 26.5.2, Apple Clang 17.0.0, Python 3.14.4. All timing comes from this one machine; other Apple Silicon generations are untested. See [SETUP.md](SETUP.md) and the [203 build record](research/final/203-cpi-memory/BUILD.md).

## Data and file formats

`profiling/golden-v1/all.txt` has 12,863 unique addresses, one per line. We removed five duplicate strings from 12,868 source records. The development and validation splits are in separate text files.

The source is the 21-column Senzing CSV `profiling/golden-v1/raw/senzing-v1.2.0.csv`; its header row lists the fields, and [profiling/golden-v1/README.md](profiling/golden-v1/README.md) describes the set.

The paired benchmark TSV has no header. Each row is a lowercase two-letter country code, a tab, and an address. The matching text file has one address per line, in the same order.

Parser outputs are stored as JSONL: frozen `baseline-outputs.jsonl` files and live dumps both hold each address with its ordered `components`.

Parser outputs are comparison references. They are not ground truth. A missing or unrun measurement means unavailable, not zero.

## Verified result

| Measure | 203 vs original |
| --- | --- |
| Correctness | Byte-identical output on 14 datasets, 130,341 rows. The negative control caught a one-byte change. |
| Warm parsing speed | 6.11× golden-v1, 5.83× novel, 5.55× geometric mean over the 11 OSM sets, 5.72× v14. The cohorts were timed separately and are not pooled. |
| Startup | Slower: 9.64 s vs 2.79 s median (+6.87 s). |
| Peak memory | Lower peak physical footprint and lower peak RSS in all 42 paired runs. |
| Threads | One parser thread in all 36 live samples. |
| Large open-data test (v15) | 7,266,489 real addresses from 88 countries (OpenAddresses + OpenStreetMap). Warm 4.96× (95% CI 4.81–5.12), every country 3.83×–6.17×. Byte-identical. Lower peak memory 3/3. One thread ([206](research/experiments/206-v15-88-country-benchmark/README.md)). |
| Public API | 7 of 8 API families byte-identical on fresh builds (most on 4,356 addresses; toponym 189 pairs, lifecycle 200). Feature printing is refused by design ([204](research/experiments/204-public-api-parity/README.md)). |

These checks passed on the 130,341 rows of the 14 datasets and on the 7,266,489 addresses of v15. Finite checks do not prove behavior for every possible input.

The headline ratios are geometric means of the three per-round warm ratios. That is how the suite reports them. [METHOD.md](research/METHOD.md) defines speedup as median over median. Computed that way from the same captures, the values are 6.03× for golden-v1, 5.93× for novel and 5.72× for v14. The 11 OSM sets range from 5.48× to 5.70× ([analysis output](research/analysis/203-startup-breakeven/output.txt)). Every paired per-round ratio exceeded 1.

Steady memory was also lower on all three sampled datasets (one sample each).

The earlier CPI build (119) used more peak memory than the original on some datasets; 203 fixed that ([202 record](research/experiments/202-cpi-final-validation/FINAL_VALIDATION_REPORT.md)). Timing ran on AC power, but system load varied. For scope and raw evidence, see the [203 candidate record](research/final/203-cpi-memory/README.md), [output audit](research/audits/203-output-independent-audit-v1.md), [timing audit](research/audits/203-timing-final-independent-audit-v1.md), and [steady-memory audit](research/audits/203-steady-memory-independent-audit-v1.md).

## Startup cost

The warm ratios leave out library and model loading. The candidate builds its compressed in-memory structures at load time, so it starts slower. In the same 84 recorded captures (42 same-round pairs), setup took 9.64 s for the candidate and 2.79 s for the original (medians). That is 6.87 s longer.

Break-even is how many addresses one process must parse before the candidate's total time (setup plus parsing) drops below the original's. I computed it from warm per-address times:

| Dataset group | Break-even addresses (range over rounds) |
| --- | --- |
| golden-v1 | 262,483 – 271,644 |
| novel | 258,626 – 290,674 |
| OSM v3–v13 | 442,473 – 501,785 |
| v14 | 545,695 – 553,929 |

Across all 42 pairs the median is 477,308 addresses. Put another way, the candidate wins once the original would have spent 8.2–9.4 seconds parsing.

So the optimized build suits long-running services and large batch jobs. For one-shot command-line use on a few thousand addresses, the original finishes first. The estimate ignores the slower first (cold) pass, so it only describes the warm state. The script and per-pair output are in [research/analysis/203-startup-breakeven/](research/analysis/203-startup-breakeven/).

## Main folders

| Path | Purpose |
| --- | --- |
| `profiling/` | Versioned datasets, annotations, and frozen output references. |
| `research/final/203-cpi-memory/` | Current streamed CPI source, tested binaries, PGO profile, and build records. |
| `research/final/119-cpi/` | Preserved pre-streaming CPI source and build records. |
| `research/experiments/` | One directory per numbered experiment (000–206), indexed by the [ledger](research/experiments/README.md). |
| `research/experiments/000-baseline/artifacts/` | Preserved original binaries. |
| `research/suite/` and `research/tools/` | Validation runner, configurations, results, and helper scripts. |
| `research/audits/`, `research/analysis/` | Independent audits of the 203 result, and analyses across experiments. |
| `benchmark/` | One-command setup and benchmark script. |

The [research index](research/README.md) covers the other files in `research/`. Some are dated working notes. We kept them for the audit trail, and you don't need to read them. See also the [measurement method](research/METHOD.md) and the [experiment ledger](research/experiments/README.md). The repository does not record investigator, contact, or funding details.
