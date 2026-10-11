# Experiment 201 independent audit

Date: 2026-10-09 America/Los_Angeles  
Scope: read-only review of the preparation record, plans, and existing metadata/results.  
Workload status: no 201 build, parser invocation, test, or benchmark was run for this audit.

## Evidence checked

- Project policy and measurement protocol: `AGENTS.md`, `README.md`, `research/experiments/README.md`, and `research/METHOD.md`.
- Preparation record and hashes: `README.md`, `CPI_WORK_PLAN.md`, `MEMORY_CHECK_PLAN.md`, `raw/starting-state.json`, and `raw/document-manifest.json`.
- Prior candidate evidence: `research/experiments/199-token-span-storage/README.md`, `raw/handoff.json`, and `raw/source-manifest.sha256`.
- Existing API evidence: `research/analysis/parallel-exploration-2026-10-07/raw/api-edge-link-v1/manifest.json`, `raw/api-edge-78-finalists-v1/manifest.json`, and the CPI archive `hashes.txt`.

The starting-state record identifies HEAD `181737d30977adc9d287e3f7a1cfafd7277525d4`, lists the pre-existing working-tree changes, and says the 201 source was not modified. The document manifest records hashes for the three preparation documents and says no candidate patch was created. The 201 directory contains those plans and preparation metadata only: there is no 201 binary, source patch, parser output, counter capture, or measurement record. The ledger also records 201 as preparation only. Therefore every 201 result remains **not measured**.

## Findings

### P1 — The release comparison must include a source-matched uninstrumented CPI control

The hypothesis asks whether token spans improve CPI. The operation plan describes matching diagnostic builds, and the API plan describes shared-library memory controls, but the future uninstrumented suite command in `MEMORY_CHECK_PLAN.md` specifies the preserved original and candidate. That can show the candidate against the original acceptance baseline, but it cannot establish that token storage improved CPI itself.

Before any speed or release-memory attribution, compare uninstrumented CPI-source control and token-span candidate built with the same compiler, flags, scanner object, allocator, model, corpus order, and harness, with `EXP_TOKEN_SLICES` as their only functional source difference. Also retain the packaged CPI and preserved original roles for the absolute acceptance comparison. Report candidate-versus-CPI deltas separately from candidate-versus-original acceptance. If the operation screen is not promising or correctness/thread gates fail, leave uninstrumented speed unmeasured as the parent coordinator directs.

### P1 — A higher API-harness peak remains unresolved evidence

The existing 78-case API run records whole-process Python + library + model peak footprint: 2,301,265,960 B for preserved 000 and 2,530,331,120 B for 199 mimalloc-v2. Its candidate shared library was linked from a static archive while 000 was a preserved dylib, so this is not an attributable parser-only delta. It is still a real observed higher peak in that harness.

The preparation correctly says this API number cannot replace the suite's peak/steady footprint procedure and proposes a matched rerun plus a native runner if attribution remains unclear. Keep this issue as an explicit unresolved gate: a lower corpus-suite footprint alone does not justify a blanket memory claim or promotion while the higher API peak is unexplained.

### P2 — Label API shared-library adaptations accurately

The planned role name `cpi-packaged-link` is a new dylib linked from the preserved CPI static archive, not the preserved/package executable or a shipped CPI dylib. Rename the role to something such as `cpi-archive-mimalloc-api-link`; document its archive and link inputs. Keep packaged CPI as a separate role wherever its executable can run the suite. This avoids treating an API harness adaptation as the packaged deliverable.

### P2 — Specify structural span cases before collecting correctness evidence

The plan calls for structural edge cases but does not enumerate them. Preserve targeted cases for strictly separated spans, adjacent/touching spans, overlapping spans, ignored/separator tokens between retained spans, punctuation and multibyte UTF-8 byte offsets, and the exact old-copy fallback. Check that terminator writes cannot affect a later retained token and that view data cannot outlive the synchronous parse or survive `rs_str` reuse. Record finite coverage as evidence, not universal proof.

## Comparison interpretation

The source description is accurate: CPI's packed `cstring_array` appends token payload bytes into a growable buffer; it is not one heap allocation per token. The 199 view path can avoid eligible token payload copies while still doing the common normalized-input copy into `rs_str`, writing terminators, storing token records, scanning eligibility, and retaining container capacity. Diagnostic counters should keep payload bytes, terminator bytes, vector pushes, capacity-growth events, fallbacks, and retained high-water capacity separate. This is an operation attribution, not proof of speed.

The prior 199 all-13 memory screen is not source-matched to CPI: 199 mimalloc-v2 has no PGO and scanner `-O0`, while accepted CPI has development-trained PGO and scanner `-O1`; it also changed allocator relative to original. The previous 199 result reports no uninstrumented speed. Its output checks and one-thread samples establish evidence for that artifact and those cases only; they do not supply 201's incremental CPI comparison.

## Audit disposition

The preparation is honest about missing measurements, allocator/build confounds, and the distinction between the API harness and suite acceptance. The execution plan needs the source-matched uninstrumented suite pair above, and the API memory excess must remain open until explained. No optimization, speedup, or memory acceptance is established by this audit.

## Source review — 2026-10-09 01:29 PDT (pre-run only)

Reviewed `raw/token-storage-diagnostics-v1/source.patch`, its handoff and source manifests, the frozen 199 `source-v8` manifest, and the referenced CPI185 diagnostic archive provenance. This is a source-only review; no build, parser, model, test, or calibration workload was run.

### Source identity and scope

- The 201 derivative is based on 199 `source-v8` (manifest-file SHA-256 `7d47969a…`); it changes `address_parser.c`, `diag.c`/paired diagnostic headers, `string_utils.c`, and `tokens.c`. The 199 token view implementation and its `tokens.h` layout are retained. The patch adds measurement hooks; this review found no additional parser-semantic branch beyond the existing `EXP_TOKEN_SLICES` candidate behavior.
- The 201 source handoff v1 has a stale base archive identity. It claims SHA-256 `98275eb2…` for the CPI185 diagnostic archive; the live file at that path hashes to `c0700a8b…`, matching the 185 artifact provenance. The `98275eb2…` value belongs to the historical 199 v9 derivative archive. The benchmark owner added a metadata-only correction at `raw/token-storage-diagnostics-v2/handoff-v2.json` and `HANDOFF-v2.md`, verifying `c0700a8b…` and identifying the mix-up. V1 remains preserved with its incorrect value; all build references should use v2.
- The derivative contains `tokens.h`, `diag.h`, and `original/diag.h`; the two diagnostic headers have identical hashes. The build record still needs each translation unit's actual argv and resolved include paths/hashes, particularly the local `tokens.h`/`diag.h` pair and the CPI185 `string_utils.h`/`vector.h` definitions. The intended compile-time treatment is the same source and common 119/CPI185 flags in both modes, with `EXP_TOKEN_SLICES` as the sole mode delta; a handoff declaration is not proof of the actual command.

### Counter audit

- `cstring_array_add_string_len` appends `len` payload bytes and calls `char_array_terminate`; the copy hook counts one call, `len` payload bytes, and one NUL store. It does not call each token a heap allocation.
- The view hook counts one record, `len` referenced bytes, and one token-end NUL store. Those payload bytes are not copy bytes. Strict-gap validation happens before any view terminator write; unsafe inputs go through the old copy helper.
- Offset-vector and token-record growth use `m` before/after each corresponding push and report the resulting vector capacity multiplied by element size. Character-buffer growth reports realloc capacity bytes only for the currently tracked token-payload or normalized-text buffer. These are capacity-growth observations, not total allocator calls or physical allocator footprint; initial allocations and unrelated arrays remain outside the new counters.
- Used/capacity high-water metrics are read after token construction. Token-payload used length includes copied token NULs; normalized-text used length includes its NUL. The normalized copy hook runs once per parse in both modes. The 185 generic allocation counters are not a substitute for these hooks because `tokens.c` and `string_utils.c` are not wrapped by the parser's `diag_alloc.h` include.
- Counters are only accumulated while `diag_address_begin` has set the active flag. The control computes the span-eligibility opportunity under `EXP185_DIAG`; actual selection counters remain conditional on `EXP_TOKEN_SLICES`. This keeps diagnostic eligibility work present in both modes and the functional view choice only in the candidate.

### Calibration still required

The handoff lists representative span shapes but has no calibration program or expected-counter equations. Before interpreting corpus totals, preserve a small calibration record proving these identities across both modes:

1. Candidate view records + fallback copy calls equal the control's copied retained-token calls; candidate view bytes + fallback-copy bytes equal control copy bytes; candidate view NUL stores + fallback-copy NUL stores equal control copy NUL stores.
2. Candidate opportunity eligible/fallback addresses equal its actual selected eligible/fallback addresses; control opportunity counts cover every parser-entered non-NULL calibration address.
3. Normalized-copy calls, payload bytes, NUL stores, and normalized-text capacity observations agree between the modes for the same sequence.
4. Vector growth-event counts correspond to observed capacity changes at the push sites, and requested-byte units are explicitly array-capacity bytes. Character-buffer growth is verified only with the required `EXP_FAST_APPEND` path active.

Also record which counters are deliberately zero or not applicable in the no-slice control. Existing source definitions are clear about payload versus NUL units; the equations and calibration evidence are needed to show the counters were wired to the correct build and capture path.

This source review finds no concrete behavior or counter-placement defect from inspection alone. Correctness, calibration, byte identity, thread count, operation totals, memory, and timing remain unverified until the benchmark owner captures them.

### Paired diagnostic build metadata — 2026-10-09 01:35 PDT (no parser review)

Inspected `tools/build_token_diagnostic_pair.py`, `raw/token-storage-diagnostics-v2/build-pair-v3/build-summary.json`, and both `artifacts/token-diagnostics-v4/{control,candidate}/hashes.json`. These are captured build records from the benchmark worker; I did not run the helper or invoke either dumper.

- The build summary verifies the corrected CPI185 archive SHA-256 `c0700a8b…`, source manifest `d19c0e2d…`, and source tree `8ac38d45…`. Every manifested source file passed the helper's hash preflight.
- Both compile modes record the same common 185 variant-flag list, include order, source manifest, and base archive. The only compile-mode delta is `-DEXP_TOKEN_SLICES`. Object hashes for `tokens.o`, `string_utils.o`, and `diag.o` are identical; only `address_parser.o` differs, as expected for the flag-controlled route.
- The archive member inventory includes all four replaced members: `address_parser.o`, `tokens.o`, `string_utils.o`, and `diag.o`. The helper replaces those members, indexes the archive, and links a dumper with `-O2 -g -DEXP185_DIAG`. No token parser or model execution is evidenced by the build summary.
- Include-path review: the derivative tree intentionally supplies its manifested `address_parser.h`, `tokens.h`, and diagnostic headers. The build falls through to `libpostal/src` for `string_utils.h`, `vector.h`, `token_types.h`, and base headers; these files' hashes match the 185 CPI source inventory. The base address-parser header itself differs only by the derivative's added `#include "tokens.h"`; the matched derivative header is manifest-pinned.

The earlier pair-v2 flag failure and v1 hash/preflight failures remain preserved as failed attempts, not validation evidence. No blocker was found in the v3 build metadata inspected here. The calibration program/equations are still missing, and the output hashes in build metadata are binary hashes only; output correctness and counter validity are not established until explicit parser-side captures and the cross-mode calibration checks are recorded.
