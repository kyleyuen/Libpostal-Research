# Research outcome report — outline for completed validation 202

This is a reporting outline, not a result. Fill every numeric claim from the same final 202 run and link its raw evidence. Do not infer performance from operation counts or mix historical runs.

1. **Goal and result** — one-thread speed target; exact ordered output; peak and steady memory requirements; one verified 202 table with medians, spread, per-dataset values, and exact run path.
2. **What changed in the accepted CPI** — source-verified table/hash lookups and model-derived records; token/word handling; component-phrase ID table; scoring and decoder specializations; exact scope and fallback paths.
3. **Build and runtime** — exact accepted archive/executable hashes, Clang version, build flags, PGO input/profile identity, LTO, scanner flags, mimalloc object, model identity, machine and environment.
4. **Validation** — dataset identities/counts (all 13 are observed); exact output comparison and frozen references; one-thread samples; fuzz input count and skipped invalid inputs; API case registry selection and exclusions.
5. **Timing** — state the suite version, sequential order, rounds/passes, eligibility/gates, per-dataset median/range/CI, overall weighting, load time, and any affected trials.
6. **Memory** — peak and steady footprint with exact paired roles and run identity. Keep RSS, Python-host API capture and native C API capture distinct from parser-suite footprint.
7. **Engineering work** — explain model-derived lookups, exact token/string work reductions, one-thread SIMD, and build profile in plain language. No OpenMP, worker pool, or multithreaded parsing claim.
8. **Exploratory work** — summarize the 201 token-view candidate and 198/200 as diagnostic research only, with their real limitations and no incremental benefit claim unless a matched result proves it.
9. **Limits** — finite corpus/fuzz/API scope, invalid UTF-8 or unsafe API exclusions, platform portability, and any incomplete public API/lifecycle checks.
10. **Reproduction and evidence** — direct links to 202 report, suite README, method, archive manifests, source patches, raw logs, and current research overview. Preserve failures as historical evidence.

## Evidence needed before finalizing

- Completed 202 `report.md` plus machine-readable results and run manifest.
- Exact baseline/candidate binary, source, model and input hashes.
- Timing trials, environment snapshots, correctness outputs/reference checks, fuzz manifest, thread captures, `/usr/bin/time -l` peak and `footprint -p` steady logs.
- Any final API/native-memory capture and its registry/safe-case limitations.
- Audit201’s final document review of the numeric and correctness statements.
