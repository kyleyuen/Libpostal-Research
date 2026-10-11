# Operation-based research measurement plan

Effective: 2026-10-06, for new experiments after 183 (184 onward).
Status: documentation approved by the user; instrumentation and experiments not started.
Update 2026-10-07: [184 runner preparation](../experiments/184-operation-measurements/README.md)
contains the collection script/config and deferred checks. No execution or build;
diagnostic binaries and instrumentation remain unprepared.
Authority: `../METHOD.md`. Historical measurements retain their original protocol.

## Purpose

Use measurements of actual work to choose architecture changes. Runtime shares
identify expensive stages, but do not establish which work is necessary or
redundant. Operation counts also need cost and correctness evidence: fewer
lookups can still mean more copying, larger tables or slower searches.

Two forms of evidence serve different purposes:

- **Research diagnostics:** operation counts, work distribution and separately
  collected profiles to explain behavior and test an architectural hypothesis.
- **Final benchmark:** the existing original-versus-candidate protocol for
  throughput, cold/warm behavior, setup, output, threads and peak/steady memory.
  This remains the basis for professor-facing results and promotion.

## Planned sequence

1. Inventory existing counters/oracles before adding instrumentation. Reuse
   documented measurements where their definitions and build identities match.
2. Define a common metric schema and exact counting locations for the original,
   cpi and 181 prototype. Include a future hybrid only once one exists.
3. Establish diagnostic baselines on development inputs with matching workloads
   and build settings. Preserve the original library; instrument a separately
   identified original-source derivative, never the archived original artifact.
4. Locate repeated work across stage boundaries. Propose one change with a
   predicted operation reduction, accounting for added storage and fallback work.
5. Test the prediction with a matching unchanged-source control and a focused
   ablation. Keep structurally promising results even when early timing is flat;
   record failures and avoid treating count reductions as measured speedups.
6. Validate the selected candidate on all 13 existing datasets, targeted
   integration tests and declared fuzz workloads. They are regression datasets;
   v12/v13 were spent in 183. Use newly frozen data for any new unseen-data claim.
7. Run the existing final benchmark on uninstrumented builds, against the
   preserved 000 original and cpi in the same session. Promote only after all
   output, single-thread, peak/steady memory and speed requirements pass.

## Diagnostic metric definitions

| Family | Measurements | Required interpretation |
|---|---|---|
| Lookups | Actual calls by dictionary/model and stage; hits/misses; repeated logical queries; answers reused | Distinguish a requested answer from an executed search; a moved search still counts |
| Search traversal | Hash probes, key comparisons, bytes compared, trie/tree nodes or edges visited | Name the structure and counting boundary; probe units differ across structures |
| Text | Keys/strings constructed; bytes normalized, appended, copied, hashed or compared | Identify intermediate versus output text; avoid double-counting shared work |
| Allocation | Alloc/free/realloc calls, requested bytes, buffer growth and temporary high-water bytes | Requested/cumulative bytes are distinct from live bytes and OS footprint |
| Model access | Feature IDs resolved, misses, rows visited and weights read/accumulated | Preserve event order and exact semantics; repeated IDs may be required contributions |
| Filters/fallbacks | Checks, passes/rejections, searches avoided, fallback frequency and work | Charge filter and fallback overhead; verify rejection is semantically sound |
| Hardware/profile | Available instructions/cycles, cache misses, stalls and stage samples | Separate profiling session; unavailable counters marked unavailable, never inferred from stack share |

Every counter must document its unit, scope, increment point, reset/lifetime,
denominator and availability for each build. Aggregate raw totals by corpus,
stage and feature family, with per-address/per-token rates and useful
distributions. A bounded trace may investigate duplication; do not make large
per-address logs the default. Never use diagnostic input statistics to generate
parser lookup tables, result caches or model layouts.

## Measurement safeguards

- Compile counters and trace storage out of final timing/memory builds.
- Check diagnostic output against its uninstrumented sibling and the original;
  include a negative control and verify counter coverage with a small known case.
- Report added diagnostic memory and profiling overhead separately. Do not
  subtract estimated overhead from instrumented timings to claim release speed.
- Compare the entire pipeline. Experiment 180/181 moved a word-model lookup into
  context fill; routed-event counts alone do not prove a lookup was eliminated.
- A cpi/prototype hybrid must preserve cpi's existing sharing and optimized
  routines; measure newly added bookkeeping rather than assuming benefits add.
- Record missing coverage explicitly. Do not equate unavailable measurements
  with zero or invent a single score for architectural optimality.

## Evidence and cost

Each new experiment uses the updated template: predicted operation changes,
metric definitions, raw counts, control/ablation, correctness, tradeoffs and a
separate final-benchmark status. Preserve commands, timestamps, exits, patches
including new files, build/input/model hashes and failed attempts.

When implementation is later authorized, use Luna/Terra for bounded research and
routine verification; delegate only genuinely independent work. Timing remains
serial. No builds, instrumentation, experiments or benchmark runs were authorized
by this documentation request.
