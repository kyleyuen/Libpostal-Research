# Measurement and evidence protocol

## Comparison contract

One-thread address parsing with the same model, input bytes, input order and
API options. Preserve ordered parser labels/components. Compare both speed and
memory with the same harness and workload. State any compiler/flag change as part
of the experiment. No memoization shortcut based on knowing the frozen inputs.

The target is 3.5× throughput. Calculate:

`speedup = median(candidate addresses/s) / median(baseline addresses/s)`

`memory difference = max(candidate peak RSS) - max(baseline peak RSS)`

For equal work, 3.5× throughput corresponds to approximately 71.4% less elapsed
time. Report actual measurements even if the target is missed.

## Each experiment

1. Create a numbered directory before changing code. Write the hypothesis and
   capture the starting commit/state. Store the final patch or commit, including
   newly created source files; preserve an executable or enough build evidence
   to reproduce it.
2. Use development inputs for routine tuning. Keep final validation measurements
   separate. The complete corpus was already measured during baseline setup;
   do not describe validation as a never-observed or training-disjoint test set.
3. Record exact build/run commands, stdout, stderr, exit codes, versions, flags,
   hashes, timestamps and notable interruptions. Keep failed builds and trials.
   Historical setup logs are incomplete for some commands; do not invent missing
   records. Future experiments should capture them as they occur.
4. Run exact output comparisons and tests appropriate to the change. A mismatch
   means the behavior-preserving claim fails unless investigated and separately
   authorized as a behavior change. Never overwrite the baseline reference.
5. Time baseline and candidate sequentially, preferably alternating their order,
   on the same machine and session. Use at least three trials each (preferably
   five for final reporting), at least ten seconds per trial, same warm-up and
   corpus. Run no profiler or competing benchmark during timing. Retain all
   results, explain any exclusion, and report spread as well as median.
6. Measure peak RSS using the same mechanism and compare steady-state memory as
   well before the final no-increase claim. Current historical evidence is peak
   RSS only. If allocator/OS noise makes small differences ambiguous, repeat and
   mark inconclusive rather than round away an increase or change the criterion.
7. Profile separately to explain measured gains. Inclusive stack shares overlap;
   they are estimates, not exact cycle counts or proof of cache misses.
8. Update the experiment ledger, activity log and professor report with evidence
   links. Keep incremental gains distinct from the combined final improvement.

## Final presentation checklist

- Concrete code changes and why they affect the measured bottlenecks.
- Original baseline and final candidate built and rerun under matching conditions.
- Throughput, variability, speedup and memory difference on declared datasets.
- Exact output checks, targeted tests, failures and limitations.
- An ablation or per-change comparison where needed to attribute combined gains.
- Frozen corpus/source/model identities, raw trials, commands and source patch.
- Explicit distinction between achieved improvement and the original 3.5× target.

## Hard acceptance constraints (added 2026-09-29 at the user's direction)

Every experiment must satisfy both, or it is rejected regardless of speed:

1. **Byte-identical output.** The ordered (label, component) pairs for all 12,863 golden-v1 inputs must be byte-for-byte identical to the
   original library's, checked with a dumper run against the baseline, the candidate and (where a rebuild is involved) an unchanged-source
   control, plus a comparison against the frozen `golden-v1/baseline-outputs.jsonl`, and a negative control proving the check can fail.
   Reference implementation: `experiments/003-batched-lookups/tools/dump_outputs.c` and its record in that README.
2. **Single thread.** Parsing runs on one thread only: no threads, no parallel loops, no worker pools, no thread-level offload, and no
   second process. Verify by inspecting the change for thread/parallel primitives and by counting the threads of the running candidate
   (`ps -M`) during a run; record the result. SIMD/prefetch/software pipelining inside the single thread are allowed; they are not
   additional threads.

## Generalization protocol (added 2026-09-29; the real evaluation uses a NEW dataset)

The final evaluation will run the reference build and ours on an address set we have not seen. So a result on golden-v1 alone is not enough:

1. **Design rule (anti-overfit).** Optimizations must be structural: derived from the code, the algorithm, or the model file's own contents.
   Never derive tables, caches, key lists, thresholds or layouts from golden-v1 inputs or their statistics, and never memoize across addresses
   (the benchmark replays the same addresses; a result cache would be inflated). Any hotness or frequency ordering must come from the model
   (e.g. the vocab counts), never from the benchmark corpus. Compile-time or profile-guided choices trained on a corpus must be trained on the
   development split only and then evaluated on validation and the novel set.
2. **Novel dataset.** `profiling/golden-v2-novel/` (12,863 strings, none in golden-v1; different component orders, casing, typos). Every accepted
   candidate must (a) be byte-identical to the baseline on it (`baseline-outputs.jsonl`) and (b) keep its speedup on it.
3. **Cold-stream check.** `research/tools/bench_ext.c` times pass 1 immediately after setup (no warm-up; caches hold only what loading touched) and
   later warm passes. A gain that exists only in warm repeated loops is reported as such. Candidate speedup is judged on the cold first pass AND warm passes.
4. **Per-country reporting.** The same tool reports us/address per country for the cold and the last pass. Report the min/median/max per-country speedup;
   a win driven by one or two countries is flagged.
5. **Setup time.** `bench_ext` reports setup seconds. Any change that adds work at load time (re-encoding or rebuilding the model) must report it.
   Steady-state footprint is judged separately from peak load footprint.
6. **Selection discipline.** Choose among variants on development; report validation, all, novel and cold results for the chosen one without
   re-choosing; keep every variant's results.
7. **Unseen-data rule (user request, 2026-10-03).** After a candidate is faster on the screening sets, re-measure it on a freshly built OpenStreetMap set
   that no step has used before (golden-v4, v5, v6, … — each used for one re-measure batch, then retired). Acceptance requires: byte-identical there,
   faster on the screening sets and not slower on the unseen set. The unseen sets share one generator, so they test new cities, not a new source.

## Environment control protocol (added 2026-09-29; the laptop is shared and noisy)

Timing on this machine is affected by other applications, battery/power state, thermal state and memory pressure. macOS cannot pin a process to cores
(Apple Silicon ignores affinity hints; priority boosts need root), so the protocol controls what it can and records the rest:

1. **Hard gate** (`research/tools/env_gate.sh`, checked at session start and before every run): on AC power, Low Power Mode off, no thermal/performance warning.
   A failed gate aborts the session (`ABORT` file); it is never silently ignored.
2. **Scheduling class.** The benchmark harness (`research/tools/bench_ext.c`) sets `QOS_CLASS_USER_INTERACTIVE` for its thread. Identical for every variant,
   including the baseline. The session runs under `caffeinate -i` so the machine cannot idle-sleep.
3. **Telemetry per run** (`research/tools/env_snapshot.sh`, before and after each run, plus a `top` CPU sampler during it): battery %, charging state,
   swap use, memory-free %, load average, system CPU usage and the busiest processes. Per pass: involuntary context switches and CPU time vs wall time.
4. **Robust statistics.** Many short passes per process, pooled over processes; report the median, the fast-tail P10 (interference-resistant estimate), CPU-time medians,
   and "clean" passes (no involuntary context switch, wall within 2% of CPU time). Variants are interleaved and rotated so slow drifts hit all of them equally.
5. **Baseline and candidates are always measured in the same session** so a re-baseline never mixes conditions with an old number.
6. **Memory** is judged by steady-state and peak *footprint* (not RSS, which swings with swap) from `/usr/bin/time -l` and `footprint -p`.

## Existing integrity check

From the workspace root:

```sh
cd profiling
shasum -a 256 -c SHA256SUMS
```

This verifies the 70 original archived artifacts. Preserve that checksum file
and those files; use separate integrity manifests for new experiments. The
organization snapshot under `research/ORGANIZATION_SHA256SUMS` covers the new
documentation and copied baseline files at the time they were organized. Docs
will evolve; retain dated copies/manifests when preparing a submission.
