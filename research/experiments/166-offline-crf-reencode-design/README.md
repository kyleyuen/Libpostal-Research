# Experiment 166 — exact offline CRF re-encoding into a new runtime representation

Date/time/timezone: 2026-10-05, America/Los_Angeles (bounded diagnostic probe; exact timestamps, command, stdout, stderr and exit are in `raw/`).  
Status: **diagnostic design study; no candidate built or timed.**

## Hypothesis and scope

An offline compiler can copy every model row and vocabulary count bit-for-bit into a different runtime representation, preserving feature-hit order and final parser output. This is an exact re-encoding, not a new predictor. It could only be considered further if a new representation beats cpi while staying single-threaded and within the memory cap.

The probe reuses model-derived artifacts only. It verifies immutable v1 oracle hashes, counts the original O3i feature decisions, and inspects experiment 160's already-recorded generator and output checks. It does not load a model, build a candidate, time parsing, or open reserved v12/v13.

## Bounded data-backed probe

`tools/probe.sh` records its command, timestamps, stdout, stderr and exit code. The source identity is `git HEAD`; input hashes are printed in its stdout.

The existing offline generator reported 10,025,721 records, 13,557,097 slots and 3,694,560 vocabulary entries. Its read-back check found **13,557,097 bit-identical rows, 0 mismatched, 0 unmapped**, with all vocabulary counts equal. The same build's O3i stream was identical to the original on golden-v1, novel, OSM v3 and fuzz-s1-200k; its full output verifier recorded byte-identical outputs across the non-reserved suite, two frozen references and 2×2M fuzz.

The direct oracle counter in this record establishes the workload scale and hit/miss population without running the model. It is falsifiable: any changed oracle hash, line count, hit/miss count, row mismatch, or output mismatch invalidates the exact-encoding claim.

## Constraints, prior result, and decision

Hard constraints remain byte-identical golden-v1 output and one parsing thread. The compiler must derive data solely from the model and may not cache across addresses. An offline artifact must also be shipped or generated within the memory contract.

Experiment 160 was the closest implementation: its exact 160b/160c record paths were **slower than cpi** (warm ratios 0.710/0.665 and 0.833/0.822 on golden-v1/OSM) and were killed. Thus semantic feasibility is demonstrated, while this record-format/runtime design is rejected for performance. No speedup is claimed here.

Next falsifiable probe: a different offline representation must first prove row bits, O3i equality on all four v1 corpora, byte-identical parser output on the required non-reserved suites, one thread, and a pre-registered same-session gain over cpi before any milestone validation.

## Decision

**Rejected as a follow-up to the existing compiled-record runtime; exact re-encoding itself is feasible.** No production source, datasets, ledger, activity log, report, baseline, or frozen references were modified.
