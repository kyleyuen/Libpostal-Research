# Testing the optimized parser yourself

This page is for anyone checking the result independently, a person or an AI agent, on an Apple Silicon Mac. Every command runs from the repository root. Each step ends with a file that says PASS or FAIL in plain words, and the command's exit status is 0 only when nothing failed.

You need macOS on Apple Silicon (M1–M4), Xcode Command Line Tools (`xcode-select --install`), Homebrew, and about 3 GB of free disk. Add `--deps` to the first build command if you want it to install the Homebrew packages it needs. Plug in the laptop: timed runs wait for AC power unless you add `--no-gate`, which makes timings noisier.

## 1. Check the recorded evidence (under a second)

```sh
python3 research/tools/verify_203_clone_evidence.py
```

This prints `PASS` if the published outputs, timing captures, memory pairs, and thread samples are intact and consistent. It does not run the parser.

## 2. Test on your own addresses (the main test)

Make a text file with one address per line, for example `v15.txt`. Any source works: OpenStreetMap, a CSV column, your own list. If you have country codes, a TSV of `country<TAB>address` also works. The parser never sees the country; it is used only to break timings down by country. Use UTF-8. A few thousand addresses or more gives stable timings, though correctness counts at any size. Your file is copied into the work directory and the repository's datasets are not touched. To build such a file from OpenAddresses, Overture or OpenStreetMap exports, use `benchmark/build_dataset.py` (see its `--help`); [experiment 206](research/experiments/206-v15-88-country-benchmark/README.md) shows a full 88-country example.

```sh
./benchmark/run.sh --dataset /path/to/v15.txt --name v15 --api
```

The first run downloads libpostal and its model (about 810 MB) and builds both versions, which takes about 8 minutes of building plus the download. Later runs reuse the build. A full run on 2,555 addresses with `--api` took 17.6 minutes with the build already present; 5 of those minutes were spent waiting for a busy machine to quiet down. For a quick sanity check first, add `--quick`; for a 300-address file that took 3.6 minutes with the build already present.

**How long a run takes** (measured on the M4 Pro, build already present):

| Dataset | Command | Time |
| --- | --- | --- |
| 300 addresses | `--quick` | 3.6 min |
| 2,555 addresses | full, with `--api` | 17.6 min, 5 of them waiting for a busy machine to settle |
| 7,266,489 addresses (v15, [206](research/experiments/206-v15-88-country-benchmark/README.md)) | full, no `--api` | about 33 min: correctness 3, timing 20, memory and threads 11 |

Small files still take several minutes, because timing runs enough passes to measure at least about 12 seconds per run. Start with `--quick` to check everything works, then do the full run. Closing other apps before the run gives steadier timings.

Each run does the following, comparing the original libpostal with the optimized build on your file:

- **Correctness**: parses every address with both builds and requires byte-identical output. It also runs two 2-million-input fuzz streams and a negative control that must detect a one-byte change.
- **Speed**: three rounds of timed warm parsing. It reports the speedup with a 95% interval, plus startup time and the break-even address count.
- **Memory**: peak RSS and peak physical footprint in every round, which must not exceed the original.
- **Threads**: live thread samples while parsing, which must show one thread.
- **API** (with `--api`): every public libpostal API family run on your addresses, which must give identical output.

Results go to `research/suite/results/<time>-v15/`:

| File | What it is |
| --- | --- |
| `VERDICT.md` | One table of PASS/FAIL checks and the per-dataset speed, startup and break-even. Read this first. |
| `verdict.json` | The same, machine-readable. |
| `report.md` | Full suite report: per-country speedups, intervals, memory table. |
| `timing/`, `correctness/`, `api/`, `env.txt` | Raw outputs, logs, and the machine state recorded before every timed run. |

**Expected result.** `# Verdict: PASS`. If the machine runs out of memory and starts paging, peak-RSS readings become unreliable and the memory check says INCONCLUSIVE (exit status 3); free memory by closing apps and rerun. Otherwise expect (with `--api`: `PASS with 1 known limitation`). Correctness, speed, memory and threads should pass. The `api` check shows KNOWN LIMITATION when the only difference is the documented one, `print_features`; any other API difference is a FAIL. That is a debugging switch for printing internal parser features, and the optimized build deliberately stops with an error on it ([experiment 204](research/experiments/204-public-api-parity/README.md)). One other family, `toponym`, crashes in the original libpostal too; the verdict reports that it crashed identically in both builds.

## 3. Optional: only the public-API comparison

```sh
./benchmark/run.sh --api-only                             # 4,356 repository addresses, about 2 minutes after the build
./benchmark/run.sh --api-only --dataset /path/to/v15.txt  # your addresses
```

## 4. Optional: rerun the published campaign

`./benchmark/run.sh` with no dataset reruns the suite on all 14 repository datasets, the way the published numbers were produced. It is much slower than a single-dataset test. See [METHOD.md](research/METHOD.md) for how the published runs were scheduled.

## Reading the numbers fairly

- Speedups are for warm parsing: model loaded, many passes. The optimized build starts about 7 seconds slower, so it only finishes sooner overall once one process parses a few hundred thousand addresses. The verdict reports this break-even for your data.
- All published timings come from one Apple M4 Pro. M1, M2 and M3 Macs have not been tested. Your µs per address will differ, and so may the speedup, because the build and its PGO profile were tuned on the M4 Pro. Compare the two builds on your own machine, which is what the verdict does; don't compare your numbers with the published ones.
- PGO (profile-guided optimization) was trained on the repository's golden-v1 development addresses. Your own dataset was never used for tuning, so it is a fair out-of-sample test.
- Byte-identical output on your file shows the two builds agree on those inputs. It does not prove agreement on every possible input.

## Troubleshooting

| Symptom | Fix |
| --- | --- |
| "settle: waiting up to 300s for 1-minute load" | The machine is busy. Close heavy apps; after 5 minutes it continues anyway and records the load with each run. |
| Stuck at "waiting for the environment gate" | Plug in AC power, or rerun with `--no-gate` (timings will be noisier). |
| "WORK_DIR must not contain spaces" | Use the default location, or set `WORK_DIR` to a path without spaces. |
| Slow download | Reuse an existing libpostal v1.0.0 model with `--model-dir DIR`; it is still checked against recorded hashes. |
| "not valid UTF-8" | The message lists the bad line numbers; fix or remove those lines. |
| Want a clean rebuild | Add `--rebuild`. |
| Any step fails | The message names a log file under the work directory (default `~/.cache/libpostal-speedup/logs/`). |

## Prompt for an AI tester

```text
You are reviewing a libpostal parser-speedup project on an Apple Silicon Mac. From the repository root:
1. Run `python3 research/tools/verify_203_clone_evidence.py` and report its result.
2. Build a fresh test set of at least 2,000 real-world addresses that is NOT taken from this repository
   (for example from OpenStreetMap), one address per line in UTF-8, saved as v15.txt.
3. Run `./benchmark/run.sh --dataset v15.txt --name v15 --api` and wait for it to finish.
4. Open research/suite/results/<newest>-v15/VERDICT.md. Report every check, the speedup with its 95% interval,
   startup times and break-even, and whether the only API failure is print_features as documented in TESTING.md.
5. Compare with the claims in README.md. Separate what you measured from what the repository claims,
   and list anything that failed or did not match.
```
