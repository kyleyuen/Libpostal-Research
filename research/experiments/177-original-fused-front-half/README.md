# Experiment 177 — original-source fused front-half identity record

Date/time/timezone: 2026-10-05 PDT, pre-registered before source editing.
Status: prototype verified for oracle and final-output preservation; no performance conclusion.

## Hypothesis and change

Original libpostal normalizes tokens, performs three phrase searches, then feature generation re-fetches normalized words and phrase memberships from separate context arrays. This prototype creates one bounded reusable front record after all three phrase searches, containing the normalized-word pointer and address/component/postal phrase memberships for every token. The feature callback consumes that record for its direct word and phrase identity reads, falling back to the original arrays if allocation is unavailable. The model, normalization, phrase tries, feature construction, scoring and decode remain original.

This is a vertically connected front-half prototype from the original source, not a cpi lookup cache. It carries token and phrase identity from context fill to feature generation in one address flow. No throughput benchmark or speed claim is permitted.

Base: `181737d30977adc9d287e3f7a1cfafd7277525d4`; oracle source base is experiment 142's original-source instrumentation. Required checks: O1/O2/O3i/O4 exact comparisons, preserved-original final output checks, one-thread evidence, and recorded memory. Memory is diagnostic only for this intermediate prototype.

## Reproduction

The first build failed because the copied 142 build script still pointed at its `src/` directory; that failure and its stdout/stderr/exit are retained as `raw/build.*`. `tools/build.sh` was corrected to use this experiment's `sources/`, and build 2 succeeded with `-DEXP_ORACLE -DEXP_FUSED_FRONT`. Every model load used `research/tools/model_slot.sh`. Commands, start/end timestamps, source patch, source/binary/output hashes and raw outputs live in `raw/`.

## Checks

Full golden-v1 oracle comparison passed against the original 142 oracle stream: O1, O2, O3i and O4 each have `cmp_exit=0` in `raw/oracle/summary-2.txt`. This includes O3i's ordered duplicate/miss stream and O4's score-bit stream. Candidate final output byte-compares with experiment 142's preserved original control (`raw/final-summary.txt`, exit 0).

Independent runs on novel and OSM v3 also pass O1, O2, O3i and O4 exactly: `raw/oracle/novel.summary-2.txt` and `raw/oracle/osm-v3.summary-2.txt` each record four `cmp_exit=0` results. Their final outputs compare byte-for-byte with experiment 142's preserved original controls; the direct command results are retained in `raw/oracle/novel.final.jsonl` and `raw/oracle/osm-v3.final.jsonl`. A deterministic seed-1 fuzz run over 200,000 generated inputs also matches the preserved original prefix (`raw/fuzz-200k.summary`). The first attempted combined novel/OSM runner mishandled paths containing spaces; its failed output is retained rather than replaced.

The diagnostic development memory process reports max RSS 2,381,414,400 bytes and `/usr/bin/time -l` peak footprint 1,278,240 bytes in `raw/memory-summary.txt`. The footprint value conflicts with known direct-footprint measurements, so it is retained without a no-increase claim. No timing protocol or speed claim was run. The changed source contains no threading primitive; this prototype only uses the parser's existing single call flow. Two live `ps -M` attempts are retained: one did not resolve a PID (`raw/thread.exit`), and the other sampled `model_slot.sh` rather than the candidate executable (`raw/thread2.psM.txt`). Thus no valid runtime thread-count proof exists for 177.

## Architecture lesson and next step

The original architecture can move a compact per-token identity record across the context/feature boundary while preserving all stage traces. The next integration should make the record context-owned, then use it for the remaining repeated phrase and context-word consumers; retain the same O1–O4 gate after each expansion. Do not introduce a second model representation or an input-derived cache at this stage.

## Decision

Keep as a verified structural prototype only. It establishes an auditable original-source boundary for a fuller fused front half; it is neither memory-qualified nor benchmarked. Ledger, activity log and professor report are deliberately untouched by this worker.
