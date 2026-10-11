# Libpostal address parser optimization

This repository aims to speed libpostal’s default parser while preserving ordered labels and components, keeping parsing single-threaded, and measuring memory. The current CPI candidate adds a streamed postcode-graph loader, avoiding simultaneous residency of the full graph index and its compressed representation during setup. A public feature-print API probe aborted in 119 CPI; 203 ran no full API suite, so parity across public APIs is unverified.

## Get the repository

This GitHub repository is private. You need collaborator access before cloning; credentials are not stored here.

```sh
git clone https://github.com/kyleyuen/Libpostal-Reserach.git
cd Libpostal-Reserach
```

For a quick review, follow [REVIEW_GUIDE.md](REVIEW_GUIDE.md). To prepare a local Apple Silicon macOS environment, follow [SETUP.md](SETUP.md). The current candidate and tested scope are summarized in the [promoted 203 build record](research/final/203-cpi-memory/README.md); the [203 experiment record](research/experiments/203-cpi-memory-regression/README.md) preserves the diagnosis and raw validation paths. The [202 record](research/experiments/202-cpi-final-validation/FINAL_VALIDATION_REPORT.md) remains the historical result for the earlier CPI build.

For setup, run `./benchmark/run.sh --setup-only`; append `--deps` if the helper should install missing Homebrew packages. It builds from pinned source with the current 203 candidate by default; `--cpi-source 119` selects the preserved pre-streaming CPI source for historical comparison. Setup prepares source/model, fresh binaries, and a suite config, then stops before the suite validation run.

To verify the evidence included in a clone without rerunning the benchmark, run `python3 research/tools/verify_203_clone_evidence.py` from the repository root. It checks the promoted package, output files, timing records, paired memory comparisons, and live thread samples. It does not validate the external model files.

## Methodology and hardware/software

Across 14 datasets (130,341 rows), candidate dump JSONLs were byte-compared with cached original JSONLs. Thirteen caches predated 203; the v14 original cache came from 202’s fresh original run. Input and artifact hashes were checked, as were frozen golden-v1 and novel outputs; the negative control detected a deliberate one-byte mutation. Two 2-million-case candidate fuzz jobs matched preserved original digest/status references; original fuzz was not rerun. These checks do not cover every possible input.

Timing used three rounds and at least 10 seconds of parsing per capture: 200 passes for Senzing, 420 for OSM, and 2,300 for v14. Runs were gated on AC power and captured load telemetry; load varied. Parsing stayed single-threaded in representative live samples on golden-v1, OSM v3, and v14. The [measurement method](research/METHOD.md) describes gates and calculations.

The promoted 203 build uses Apple Clang 17, `-O2`, full LTO, the existing whole-library PGO profile trained on golden-v1 development data, an `-O1` tokenizer, and mimalloc 3. Its libpostal source is pinned to commit `25099c506612b34b23b1bfe286ca6321fcf06f35`; model data is libpostal v1.0.0 and checked against recorded SHA-256 hashes. The 202 capture reports 28,160 MB total memory but does not identify hardware, OS, or Python version. A separate 2026-10-05 audit records Mac16,8, macOS 26.5.2, Apple Clang 17.0.0, and Python 3.14.4; older golden-v1 results identify an Apple M4 Pro. These are separate records, not a complete per-run machine specification. See [SETUP.md](SETUP.md) and the [203 build record](research/final/203-cpi-memory/BUILD.md) for provenance.

## Data and file formats

`profiling/golden-v1/all.txt` contains 12,863 unique addresses, one per line; five duplicate strings were removed from 12,868 source records. Development and validation splits are in separate text files. The source CSV at `profiling/golden-v1/raw/senzing-v1.2.0.csv` has 21 fields: `record_id`, `full_address`, `house`, `house_number`, `road`, `po_box`, `unit`, `level`, `staircase`, `suburb`, `city_district`, `city`, `state_district`, `state`, `postcode`, `country`, `world_region`, `category`, `near`, `country_code`, and `source`.

The paired benchmark TSV has no header and each row is a lowercase two-letter country code, tab, and address; the matching text file has one address per line in the same order. Annotated records JSONL uses `id`, `address`, `country_codes`, `source_rows`, and `split`. Frozen `baseline-outputs.jsonl` uses `id` and ordered `components` label/component pairs; live dump JSONL uses `address` and `components`. Parser outputs are comparison references, not ground truth. Empty CSV cells remain empty strings and their real-world meaning is unspecified. Cities with no usable OSM addresses add no row; missing or unrun measurements are unavailable, not zero.

## Verified result

The checks above passed on the 130,341 tested rows; a deliberate one-byte mismatch negative control confirmed the comparator detects a changed output. Finite checks do not prove behavior for every possible input.

Warm parsing ratios were 6.11× for golden-v1, 5.83× for novel, 5.55× geometric mean across the 11 OSM datasets, and 5.72× for v14. These separately run cohorts are not combined into one speed estimate. CPI startup was slower. All 42 same-dataset, same-round pairs had lower peak physical footprint (reductions of 502,775,976–517,472,400 bytes) and lower peak RSS; representative steady footprint was lower on all three sampled datasets.

The 202 check of the older 119 CPI build passed its global maximum comparison by 70,500,184 bytes, but all six Senzing paired peak-footprint comparisons regressed: the golden-v1 maximum was +131,236,152 bytes, and the Senzing subgroup maximum was +58,982,640 bytes (novel, round 2). The streamed 203 candidate removes that measured per-dataset issue across its 42 pairs (84 captures). Timing ran on AC power, but system load varied. The known public feature-print API abort remains a limitation of the earlier CPI build, and 203 did not run a full API suite. See the [203 candidate record](research/final/203-cpi-memory/README.md), [output audit](research/audits/203-output-independent-audit-v1.md), [timing audit](research/audits/203-timing-final-independent-audit-v1.md), and [steady-memory audit](research/audits/203-steady-memory-independent-audit-v1.md) for scope and raw evidence.

## Main folders

| Path | Purpose |
| --- | --- |
| `profiling/` | Versioned datasets, annotations, and frozen output references. |
| `research/final/203-cpi-memory/` | Current streamed CPI source, tested binaries, and build records. |
| `research/final/119-cpi/` | Preserved pre-streaming CPI source and build records. |
| `research/experiments/000-baseline/artifacts/` | Preserved original binaries. |
| `research/experiments/119-component-phrase-ids/artifacts/cpi/` | Historical 119 CPI binaries and harnesses. |
| `research/suite/` and `research/tools/` | Validation runner, configurations, results, and helper scripts. |

See the [measurement method](research/METHOD.md) and [experiment ledger](research/experiments/README.md). Investigator, contact, and funding details are not recorded in the repository materials.
