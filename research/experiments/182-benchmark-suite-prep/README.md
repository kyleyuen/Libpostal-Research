# Experiment 182 — original-vs-release benchmark preparation

Date: 2026-10-06 PDT. Status: **prepared; no benchmark/timing/model-heavy run performed**.

## Scope

Audit the existing suite and stage a per-run 3-way config path: preserved experiment-000 original vs accepted cpi vs a designated release-candidate harness directory. This does not change `benchmark/run.sh`, `research/suite/suite.json`, dataset files, references, shared ledger, or activity/report files. The suite runner writes run outputs under shared `research/suite/results`; do not execute it until the release harness location is confirmed and timing is authorized.

## Findings

- `benchmark/run.sh` is fixed to building a fresh upstream original plus `research/final/119-cpi`; it cannot select 000-baseline or a third candidate. It can run suite filters but its generated config still has two variants. It also runs default correctness jobs with concurrency 3.
- `research/suite/run_suite.py` accepts a per-run `--config`. Timing loops are serial and rotate variant order; peak RSS/footprint are read from `/usr/bin/time -l`. Steady memory samples `footprint -p` after 3 passes. Threads are sampled six times with `ps -M` during a 200-pass run. Thread count is reported but not automatically treated as a reject gate. The prepared config sets correctness jobs to 1; run candidate lists are baseline plus the two explicit candidates.
- Frozen output refs exist only for golden-v1 and novel. All datasets are otherwise compared candidate-to-original via generated JSONL hashes. The current suite records `compare_frozen.py` output but does not make a frozen-reference mismatch part of its `bad`/failure list; inspect this before claiming the hard frozen-reference check passed. Its negative control changes one byte and checks that SHA differs; it does not run the comparison utility against that altered file.
- Suite manifest hashes canonical `research/suite/run_suite.py` and `research/suite/suite.json`, rather than the provided per-run config, although the exact config is copied into a result directory. Record/hash the generated config separately.
- Dataset registry audit found all 13 TXT/TSV datasets physically available: 128,540 total rows. v12 (8,273) and v13 (9,716) remain marked `reserved: true` and are not selected by default; their files are present. Spending them requires explicit `--include-reserved`. Frozen references are only configured for golden-v1 and novel. Counts and SHA-256s are in `raw/registry-audit.stdout`.
- 000's immutable `libpostal.a` hash is `30de6ef223346735ac3035654ff0d435e5e621dccc94dc77869807dbf146fe22`; workspace source checkout is pinned at `25099c506612b34b23b1bfe286ca6321fcf06f35`. I linked the three suite harnesses against that archive, rather than rebuilding a baseline. The outputs and hashes are under `artifacts/original-000/` and `raw/harness-hashes.sha256`.
- cpi suite harnesses exist at the registry paths. Experiment 180's instrumented `event` build has `dump_outputs` and `fuzz_pipeline` but no `bench_ext`; the release harness set is now present at experiment 181's `artifacts/release/`.

## Prepared path and checks

`tools/build_original_harnesses.sh` links `bench_ext`, `dump_outputs`, and `fuzz_pipeline` against the preserved 000 archive using current checkout headers (the checkout revision is checked before linking). The compile/link succeeded for all three. This is a compatibility/build check only; no parser was run.

`tools/make_run_config.py` generated the actual 3-way config at [`run-suite-3way-181.json`](run-suite-3way-181.json), selecting preserved original-000, cpi, and experiment 181 release harnesses. Config SHA-256: `59c18cb15176954caab6babba3045b542231a96508b54ccab159683f571fef24`. `tools/verify_run_config.py` confirmed all nine executable paths, exact variant order, single-worker correctness setting, all 13 datasets (128,540 rows), TXT/TSV address alignment, and frozen-reference line counts; it did not load the parser model. Full per-input hashes and binary hashes are in `raw/verify-181-config.stdout`. v12 and v13 are available but remain flagged reserved in the config.

Exact full-suite command that includes the reserved sets (not run in this preparation task):

```sh
/opt/homebrew/bin/python3 research/suite/run_suite.py --config research/experiments/182-benchmark-suite-prep/run-suite-3way-181.json --include-reserved
```

This invokes every suite stage, including correctness, fuzz, timing, steady/peak memory, threads, and report; correctness jobs are serialized. It writes the run under shared `research/suite/results/`. An attempt to point the config generator at the 180 artifact root failed because that root lacks `bench_ext` (see `raw/config-180-missing-bench.*`).

All prep commands have local start/end timestamps, argv, stdout/stderr, and exit code in `raw/<name>.*`. Successful checks include registry audit, `bash -n benchmark/run.sh`, suite CLI help, Python bytecode compilation, original harness linking, config-generation self-check, and final 13-set/config preflight. No timed runner, dump, fuzz, memory sample, thread sample, model load, or output correctness workload was started.

## Files

- `tools/build_original_harnesses.sh`: serial harness linker, original archive only.
- `tools/make_run_config.py`: isolated three-variant config generator; default benchmark untouched.
- `tools/audit_registry.py`: read-only dataset path/count/hash inventory.
- `tools/verify_run_config.py`: final 181 config, binary and corpus preflight without model access.
- `run-suite-3way-181.json`: ready config; it does not alter shared suite defaults.
- `raw/`: command logs, registry data, generated self-check config, and binary hashes.
- `artifacts/original-000/`: newly linked harnesses against the preserved library.

## Not yet verified

No output comparisons, full correctness/fuzz suite, timing, RSS/steady-memory, or live thread counts were run for this three-way suite. Experiment 181 did perform its own bounded golden/novel/OSM output and thread smoke, but that is not the full-suite report. The runner has pre-existing correctness-gate limitations described above; do not call the prepared config a completed all-tests report.
