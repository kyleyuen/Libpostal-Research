# Experiment 186 — architecture research benchmark

Date: 2026-10-07, America/Los_Angeles. Evidence: research framework and correctness diagnostics. Status: built and verified for the declared framework/preservation scope. No architecture candidate, speed gain or memory acceptance claimed.

## Hypothesis and implementation

Explanatory measurements and a source-based behaviour map can guide structural redesign without selecting country/source-specific shortcuts. Internal algorithms, structures and intermediate scores may differ; byte-identical parser output, faster execution, memory no higher than original and one parsing thread are the acceptance requirements.

- Stdlib runner reuses experiment185 AC/process/thread capture helpers; default invocation only checks preparation. Trace collection is explicit, sequential and stored in a fresh result directory.
- Isolated original-source trace/control builds stream exact query bytes and dictionary identity, first-search/request-stage linkage and per-address scoped work summaries. Unknown dictionary generations/lifetimes do not imply safe reuse.
- Analyzer validates input identity, expected addresses, event ordering/counts and overflow, then reports per-address work and median/p95/max distributions with declared provenance strata.
- Direct API harness preserves ordered output, NULL versus empty response, explicit options, empty strings and C-string boundaries. Original responses are the oracle; mutations have no assumed output invariance.
- Structural registry: 82 cases, 78 safe API cases, four guarded/excluded cases. The [source contract](EDGE_CONTRACT.md) distinguishes source evidence, prior tests, current tests and gaps. This is not an exhaustive set of all possible inputs.

## Coverage and design

[Design and anti-overfit rules](DESIGN.md), [instrumentation scope](INSTRUMENTATION.md), [trace schema](trace-schema.json), and [metric definitions](METRICS.md).

All 13 existing corpora are observed. No cross-address result cache, corpus-derived table/threshold/layout or country-specific runtime selection. Dataset/script/source strata expose differences rather than drive parser special cases. Expected bytes come from the preserved original, not candidate outputs or manually predicted labels. Diagnostics do not impose identical internal stages or score bits on future designs.

Actual traced searches and repeat candidates are separate from complete logical feature requests. Constructor-level text copying, hash probes/trie edges, live heap, hardware counters, full value-consumer linkage and universal safe-reuse proof are not established by this first collector. Scope limitations must accompany every result. Trace overhead is never product speed/memory evidence.

## Reproduction

Starting state: [raw/starting-state.json](raw/starting-state.json). Pinned upstream original revision: `25099c506612b34b23b1bfe286ca6321fcf06f35`; preserved library under `../000-baseline/artifacts` remains untouched.

Current source/control/trace builds: [artifacts/build-20261007-final3](artifacts/build-20261007-final3). Exact commands, timestamps, flags, exit/stdout/stderr, source patches and source/binary/model hashes: [raw/build-20261007-final3](raw/build-20261007-final3). Source and new runtime files live under `source/`. Failed initial and final2 builds and intermediate builds are retained under `raw/`.

From workspace root:

```sh
python3 research/experiments/186-architecture-benchmark/tools/run_architecture_benchmark.py
python3 research/experiments/186-architecture-benchmark/tools/run_architecture_benchmark.py --mode smoke
python3 research/experiments/186-architecture-benchmark/tools/run_architecture_benchmark.py --run --mode smoke --output research/experiments/186-architecture-benchmark/results/NEW-RUN
python3 research/experiments/186-architecture-benchmark/tools/test_architecture_benchmark.py
```

The first two commands are preflight only. Execution requires AC/environment checks and rejects existing output paths. Development, full observed-corpus and structural/API workloads are explicit modes; inspect config and scope before launching. Model-loading time and traced resource overhead are provenance only, not acceptance benchmarks.

## Verification

- [Runtime calibration](raw/calibration-20261007-c): exact key identity, first physical producer/request-stage linkage, per-address counters and forced event-cap overflow. Overflow intentionally fails rather than reporting complete data.
- [API harness fixtures](raw/edge-harness-fakecheck.json): registry validation, NULL/empty distinction, no-overwrite, exact comparison positive/negative controls; these fixtures do not load the real model.
- [Real smoke](results/smoke-20261007-final3/results.json): 2026-10-07 16:57:40–16:57:52 PDT; original000, unchanged control and diagnostic derivative. Four cases (London, SW1A 1AA, empty, punctuation-only) output byte-identical, sampled one thread, four complete address summaries/74 query events/no overflow. Whole-phrase early exits now finish their summaries.
- [Runner unit/integration fixtures](raw/fixtures/unit-tests-20261007.json): 7/7 pass; output mismatch, changed generation, empty workload, p95, overflow and fake complete collection cases.
- [Golden-v1 preservation](results/golden-v1-preservation-20261007-final3/results.json): 12,863 inputs, original/control/diagnostic output byte-identical; frozen semantic comparator has zero missing/mismatched rows; negative control detects one deliberate mismatch; thread samples all one. Trace disabled for this preservation check.
- [Safe API parity](results/newedge-api-20261007-final3/results.json): 78 safe registry cases original/control/diagnostic byte-identical, thread samples one; four malformed-byte cases excluded. This lane compares outputs and does not collect work traces.
- All 13 configured corpus paths pass preparation-only preflight; their expanded traces were **not run**. Only the four structural cases have genuine per-address trace/distribution evidence in186. Final speed/peak/steady memory acceptance benchmark: **not run**.

## Decision

Framework verification is separate from architecture selection. This work provides a reusable initial collector and explicit coverage gaps, not a fastest/optimal architecture claim. Existing CPI deliverable and professor speed claims remain unchanged. Next research decisions should follow declared hypotheses about work eliminated and work added, then original-output and uninstrumented resource/speed checks.

## Source/reproduction limitations

The initial collector is implemented for the original-source control/diagnostic pair. Future alternative architectures need compatible diagnostic adapters with declared boundaries; this task does not trace CPI or181 with the new event schema. The runner is configurable but does not by itself provide missing hooks. Public API lifecycle/debug paths are mapped from source and earlier records; they were not rerun here. The 78 safe examples and historical corpora do not prove universal output equivalence. Remaining exact-lifetime/consumer, constructor/copy, traversal and live-memory metrics are explicit coverage gaps.
