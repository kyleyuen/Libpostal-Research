# Architecture research benchmark — experiment 186

## Purpose

Compare representations and information flow before selecting an architecture. A fast measured implementation is a reference, not proof of an optimal architecture. No single scalar score determines an architecture winner. Count additions as well as removals across the complete measured path; movement between stages is not elimination.

## Evidence lanes

1. **Behaviour:** original pinned source/model/API options and ordered response bytes are the oracle, including unexpected outputs. Existing corpora, original test-derived cases, structural boundary combinations, lifecycle/options and subsequently independent inputs are distinct coverage sets. Test coverage is finite; universal equivalence additionally needs a source-level argument for each transformation and exact fallbacks where its preconditions do not hold.
2. **Research:** physical searches, exact within-address repetitions, first resolution and later requests, materialized text, scoped allocations, model rows/weights, filters/fallbacks, per-address distributions and bookkeeping/storage. Every metric states the actual boundary; unsupported dimensions are unavailable, never guessed or assigned zero.
3. **Acceptance:** uninstrumented original/candidate, same input/order/options/model, single thread, byte-identical responses, cold/warm runtime, peak and steady footprint. Trace overhead is not product memory or speed. Public API compatibility needs separate coverage beyond the default text dumper.

## Anti-overfit rules

- All existing 13 corpora were observed; none is a fresh holdout. Preserve their provenance and development/validation labels.
- No address/result memoization across inputs, country-specific shortcuts, observed-key tables, frequency-driven thresholds/layouts or corpus-derived model contents.
- Architectural decisions must follow algorithm/data dependencies, exact semantics and model contents. Country/source/script strata expose regressions; they do not select special runtime paths.
- Freeze a candidate before independent validation. New inputs are versioned; never repair references to fit a candidate or repeatedly select on a holdout.
- Structural cases vary length, punctuation, scripts, repetitions, ambiguity, component order and API options. Countries and scripts are examples, not an exhaustive enumeration of behaviour.
- Metamorphic mutations are NOT assumed output invariant: obtain each mutation's expected response from the preserved original. Case-folding or whitespace changes may legitimately alter the original output.
- A missing label/category in observed outputs is not proof the original cannot emit it. Source inventory and runtime coverage are separate.

## Interpretation rules

A repeated search is a reuse candidate only when structure identity, exact key bytes, operation semantics and relevant state lifetime agree. Dictionary identity is not merely a family name. Unknown generations/lifetimes yield **unproven**, not safe reuse. Do not infer that repeated scoring contributions are removable merely because their lookup is shareable. Alternative scoring algorithms or orders are allowed when they preserve the returned output; identical intermediate events, scores or arithmetic order are not acceptance requirements. A direct/precomputed first-use ID is not a cache hit.

Per-address rates/distributions come only from genuine per-address records. Legacy 185 aggregates cannot reconstruct tails. Bounded traces must expose truncation/overflow; incomplete traces cannot support exact totals. Requested allocator bytes are not live retained memory. Instrumented source boundaries are not whole-library totals. Unchanged scoring counts do not prove scoring is irreducible.

## Reporting

Each report includes identity hashes, commands/time/exit/raw output, complete expected workload pairs, exact output/thread evidence, missing coverage, counts and distributions by declared strata, added diagnostic bookkeeping, and explicit acceptance status. No improvement is claimed from a smaller operation count alone. Existing professor benchmark remains authoritative for achieved speed/memory.

## User clarification

The replacement need not reproduce libpostal's internal steps, algorithms or data structures. Required results are byte-identical parser output, faster execution, peak and steady memory no greater than the original, and one parsing thread. Architecture-specific diagnostics explain each design; they do not impose stage-by-stage equivalence with the original. No unchanged model/score-bit representation gate is added merely because the baseline uses it. A changed representation must still establish exact observable behaviour and report its costs.
