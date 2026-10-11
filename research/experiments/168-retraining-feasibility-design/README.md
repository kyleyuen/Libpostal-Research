# Experiment 168 — retraining or smaller-model feasibility under byte-identical output

Date/time/timezone: 2026-10-05, America/Los_Angeles (bounded diagnostic probe; raw command evidence is in `raw/`).  
Status: **diagnostic design study; no model retrained, candidate built, or parsing timed.**

## Question and distinction

An **exact re-encoding** copies the current model's rows, vocabulary counts and prediction arithmetic into another representation. It can satisfy byte identity, as experiment 160's row, O3i and output checks demonstrate.

A **retrained or smaller model** creates a new predictor: the upstream trainer iterates labeled examples, can shuffle them, updates an averaged-perceptron CRF, and finalizes a new model. Even if it uses the CRF option and identical feature code, its learned rows may differ. Fewer features, rows, labels, weights, or a greedy predictor therefore have no path to proving byte-identical output merely from accuracy.

## Bounded data-backed probe

`tools/probe.sh` inspects the preserved upstream training source and current model counts, hashes each input, and extracts the prior byte-identical verifier results. The trainer source sets CRF as the default, defaults to five iterations and five minimum updates, reads a labeled training file, optionally shuffles each epoch, and calls `address_parser_finalize_model`. Current model data contains 11,470,623 state keys, 2,086,474 transition keys and 3,694,560 vocabulary keys.

The probe distinguishes 35 bundled `.tsv`/`.jsonl` resource files from labeled parser-training files by name; the former are language/category resources, not a versioned parser-training manifest. This is not proof that training data cannot be obtained; it establishes that the retained tree does not identify a reproducible labeled parser corpus for a retraining run. It does not inspect, modify, or spend v12/v13.

## Feasibility and required proof

Under this workspace's hard contract, retraining or shrinking is **inconclusive and out of scope as an optimization**: it would require a rule change or a new acceptance definition based on accuracy. Passing output checks after retraining is logically possible only if the generated predictor happens to return exactly the original label sequence on every required input; no source-level property guarantees that. The existing exact verifier covers non-reserved suites, frozen references and 2×2M fuzz, but cannot establish universal equivalence for a changed predictor.

If a professor authorizes a model-quality objective, the next probe must preserve a versioned training-data manifest and seed/shuffle/toolchain provenance, train in a separate experiment directory, measure accuracy against a declared labeled holdout, and still run the full output and one-thread/memory protocol. It must not call an accuracy result byte-identical behavior.

## Decision

**Do not pursue retraining or a smaller/new predictor under the present byte-identical contract.** Exact offline re-encoding remains the only model-side class compatible by construction; its current compiled-record implementation was already rejected on measured speed. No production source, model, datasets, ledger, activity log, report, baseline, or frozen references were modified.
