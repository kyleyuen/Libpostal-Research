# Experiment 175 — direct ordered event-flow scoring probe

Date/time/timezone: 2026-10-05T23:21:07-0700 PDT (America/Los_Angeles), pre-registered before candidate source editing.
Status: **rejected**.

## Hypothesis and change

The current cpi direct path first accumulates feature strings, resolves each to an id, then rescans per-token index ranges to score it. This experiment does **not** change key construction, feature dictionaries, compiled model records, token identities, phrase tries, or row layout. It tests only one independent architectural seam: after the existing lookups, copy each resolved `(family, token, feature-id)` event in its original order into one reusable per-address event buffer and consume that buffer in a single ordered scoring loop. Miss events are retained for the event trace but skipped by the score operation, exactly as cpi does.

The event buffer grows only when an address exceeds the current capacity and is reused for later addresses; allocation failure falls back to cpi's original two per-token scoring loops for that address. This is deliberately unlike 149's integer-key pairs and 160's compiled `(record, slot)` emitter: all normal feature construction and dictionary resolution stay unchanged.

Base commit: `181737d30977adc9d287e3f7a1cfafd7277525d4`. Source base: `research/final/119-cpi/sources` (`crf.c` SHA-256 `cc001bb7f932e3c435e59290e309b415f3c468ed53caf74eb95b4506c8048860`). Candidate source, patch, build artifacts and hashes will be retained here.

## Hard gates and planned checks

1. O3i must exactly match 142's original oracle, including duplicates and misses, and O4 score bits must match on golden-v1, novel and OSM v3. A deliberately perturbed trace must fail comparison.
2. Final output must be byte-identical to the preserved original on golden-v1, novel, OSM v3 and a fuzz sample. Parsing stays single-threaded; both static scan and `ps -M` evidence are required.
3. Peak and steady memory must be no larger than the preserved original before any candidate is considered viable. Model-loaded runs use `research/tools/model_slot.sh`.
4. This is a correctness and memory probe only. No throughput trial or speed claim is planned.

## Reproduction

Hardware, compiler, model hashes, source patch, input hashes, exact commands, timestamps, stdout, stderr and exit codes are retained under `raw/`. The preserved original is `research/experiments/000-baseline/artifacts` for comparisons.

## All measured trials

No throughput trial was run or interpreted.

| Check | Candidate | Preserved original | Result |
|---|---:|---:|---|
| `/usr/bin/time -l` peak RSS, development corpus | 2,216,050,688 bytes | 2,365,947,904 bytes | candidate lower in this one paired observation |
| `footprint -p` steady footprint | 2,189 MB | 2,171 MB | candidate **+18 MB** |
| `footprint -p` reported peak | 2,672 MB | 2,335 MB | candidate **+337 MB** |

Both 5-second footprint samples exited 0. The footprint evidence fails the no-memory-increase gate, so these are not sufficient to claim memory neutrality. The printed parser-profile rates are retained in raw stdout as harness output only; this experiment makes no speed claim.

## Correctness and interpretation

- **O4:** exact score-bit comparison passed for golden-v1 (151,312 records), novel and OSM v3 (`raw/oracle_summary.txt`; golden comparator log retained). Therefore the candidate's event-replay scoring order preserved the score rows on the screened corpora.
- **O3i:** comparison failed for golden-v1, novel and OSM v3. The first golden difference is address 1, token 4: original records state event 0 as `-`, whereas cpi's inherited `EXP_CP_IDS` shortcut records resolved id 28. This behavior predates this experiment and is not caused by its event buffer, but it means the strict O3i gate is not met by this cpi-based scratch source. The full mismatch count is 702,248/702,370 (`raw/o3_golden.compare`).
- **Final parser output:** golden-v1 SHA-256 is `45574ee2…`, exactly the original 142 control's SHA-256; novel and OSM v3 exactly match their preserved original outputs. The `profiling/golden-v1/baseline-outputs.jsonl` file has a different byte format/hash from the parser dumper, so its expected mismatch is retained in `raw/final_summary.txt` and is not used as evidence of a parser mismatch.
- **Threads:** static scan found no thread primitives in changed source; `ps -M` captured exactly one process thread for both candidate and preserved original (`raw/memory_*.psM.txt`).
- Fuzz and further corpus checks were not run after the O3i and memory gates failed. The experiment is bounded before any speed work.

## Decision

**Rejected.** The direct event buffer/replay itself preserved O4 score bits and screened final outputs, but the strict O3i comparison is not byte-identical with the original oracle on the cpi base, and steady/peak footprint exceeds the preserved original in the paired observation. Do not promote or benchmark this architecture. The experiment directory is intentionally isolated; no ledger, activity log or professor-report edit is in scope for this worker.
