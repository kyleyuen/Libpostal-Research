# Benchmark suite (original vs best version, every dataset)

`run_suite.py` runs every registered dataset ("test suite") through the **baseline** (original libpostal) and each **candidate** (by default the best version, `cpi`). It reports the **speedup multiplier per dataset** (original µs/address ÷ candidate µs/address) with a 95% confidence interval, plus correctness, memory and threads.

Requirements: Python 3.9+ (standard library only), macOS on Apple Silicon, and the binaries and model listed in `suite.json`.

## Run

```sh
cd research/suite
python3 run_suite.py                         # everything: all enabled datasets except reserved ones, all stages
python3 run_suite.py --datasets osm-v3,novel --rounds 3          # a subset
python3 run_suite.py --stages correctness                        # one stage only
python3 run_suite.py --report-only results/<run-id>              # re-analyse raw files (e.g. after editing the report code)
python3 run_suite.py --include-reserved                          # ALSO runs the never-used unseen sets: this spends them
```

Each run writes `results/<run-id>/`:

| File | Contents |
|---|---|
| `report.md` | the per-dataset multiplier table, aggregates, memory, correctness and threads |
| `results.json` | the same data, machine-readable (per-round ratios included) |
| `manifest.json` | SHA-256 of datasets, binaries, model files and the suite itself |
| `timing/` | raw `bench_ext` stdout/stderr (with `/usr/bin/time -l`) per dataset × variant × round |
| `correctness/` | candidate dumps and fuzz outputs |
| `env.txt` | power, thermal, swap, memory and load snapshot before every timed run |
| `gate.log` | output of the environment gate before each run |
| `run.log` | the run's own log |
| `suite.json`, `run_suite.py`, `invocation.json` | exact copies of what ran |

Baseline reference dumps are cached in `results/cache/`, keyed by binary hash and dataset hash.

## Stages

1. **integrity** checks that every file exists and hashes everything.
2. **correctness** requires every candidate's `dump_outputs` to be byte-identical to the baseline's on every dataset. It also compares against the frozen references where configured, and runs fuzz seeds (default 2 × 2,000,000) against the baseline. It runs up to `correctness_jobs` processes in parallel (≈2.6 GB each) and never during timing.
3. **timing** runs serially:
   - it waits until the 1-minute load average is below `settle_load_1min`, and before every run it passes `research/tools/env_gate.sh` (AC power, no Low Power Mode, no thermal warning, no build or benchmark running);
   - each run is `/usr/bin/time -l bench_ext <tsv> <passes>`;
   - the variant order rotates each round.
4. **memory** samples the steady footprint (`footprint -p`) after a few warm passes. Peak RSS and peak footprint come from the timing runs.
5. **threads** samples `ps -M` six times while parsing.
6. **report** computes the statistics:
   - the speedup per round is computed within the same round;
   - the table shows the median over rounds, a bootstrap 95% CI of that median, the min–max, the cold (pass 1) speedup, and per-country min / median / max;
   - aggregates are the geometric mean over datasets, overall and per source.

## Extend

**Add a dataset (a new source or a new unseen set).** Add an entry to `datasets` in `suite.json`:

```json
{"name": "openaddresses-us", "source": "openaddresses", "kind": "external", "reserved": false,
 "txt": "profiling/<dir>/<file>.txt", "tsv": "profiling/<dir>/<file>.tsv"}
```

- `txt` holds one address per line (used by `dump_outputs`).
- `tsv` holds `country<TAB>address` (used by `bench_ext`).
- `frozen_reference` (optional) points to a frozen `baseline-outputs.jsonl`.
- `reserved: true` keeps the set out of normal runs until `--include-reserved`.
- `enabled: false` disables the set.
- Put new data under a new `profiling/<name>/` directory, and never overwrite existing sets.

**Add a variant (another build to compare).** Add an entry to `variants` with `bench_ext`, `dump_outputs` and `fuzz_pipeline` paths, and `"enabled": true`. The example `rt8` entry is disabled. Every candidate is compared with the `baseline` named at the top of the file.

**Add a stage.** Write `stage_<name>(cfg, run)` in `run_suite.py` and add it to `STAGES`.

**Change the protocol.** Edit `settings`: `rounds`, `passes`, the settle threshold, fuzz count and seeds, bootstrap resamples and seed, and the datasets used for steady-state and thread checks.

## Rules this suite keeps (from `research/METHOD.md`)

- Nothing is ever timed concurrently. Correctness work runs before timing.
- The baseline and candidates are measured in the same session, with rotated order.
- Every number traces back to a raw file, and failed runs keep their exit codes.
- The reserved unseen sets are not spent by default.
