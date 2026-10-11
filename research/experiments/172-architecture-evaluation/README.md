# Experiment 172 — decision framework for exact single-thread architectures

Date/time/timezone: 2026-10-05T22:34:39-0700 (workspace clock; environment metadata labels the work date 2026-10-06).
Status: **diagnostic design study; no candidate built or timed**.
Hardware/OS: Apple M4 Pro, macOS 26.5.2, Darwin 25.5.0 arm64. No compiler invoked.

## Hypothesis and scope

Selecting an exact single-thread parser architecture by warm throughput alone risks choosing a candidate with poor cold performance, excessive setup cost, memory pressure, API gaps, or weak generalization. A small pre-registered decision matrix can expose these trade-offs before expensive implementation and prevent projections from being promoted as measurements.

## Proposed decision matrix

Apply hard gates first; only score a candidate that passes every gate.

| Criterion | Evidence required | Gate / use |
|---|---|---|
| Exact behavior | Frozen golden-v1 output compare + baseline library + negative control; all supported API paths | Any mismatch rejects |
| Single thread | Source primitive scan and live thread count during parse | Any extra parsing thread rejects |
| Memory | Peak footprint, max RSS, mapped resident bytes, steady footprint/RSS; same metric for original | ≤ original; unclear mapped accounting is inconclusive |
| Generalization | novel and each fresh unseen set per METHOD, selected before validation | No slowdown on unseen requirement fails |
| Cold/warm speed | Same-machine paired baseline/candidate, pass 1 and warm passes, ≥3 trials (5 for final) | Report median and spread; no corpus pooling |
| Setup and disk | Setup seconds, first-parse fault cost, extra artifact size/provenance | Report as lifecycle cost; apply predeclared user limit |
| API/lifecycle | print-features, datadir setup, repeated setup/teardown, setup failure | Any unsupported exported path blocks drop-in selection |
| Maintenance | patch size, generated artifact recipe, model-version binding, portability | Use as tie-breaker, never as a substitute for hard gates |

After gates, compare per-corpus cold and warm speed separately; report memory and setup alongside it. If multiple candidates pass, prefer a Pareto candidate that is no worse on any measured dimension and strictly better on at least one. If none dominates, preserve the trade-off table and do not collapse criteria into a weighted score without user/professor weights.

## Evidence and limits

- METHOD already specifies byte identity, one thread, corpus separation, cold/warm, memory, setup and fresh unseen checks; this matrix makes API coverage and maintainability explicit.
- 150 shows strong parsing evidence can coexist with API limitations. 158/161 repaired those; 162 showed parity but P4 promotion/validation remains separate.
- 159/160 demonstrate exactness and memory feasibility do not guarantee speed. 160's steady memory was not measured, so it cannot pass a complete memory gate.
- Comparisons must use the preserved original artifacts and same-session protocol. Historical commands/logs are incomplete in places; gaps stay labeled unknown.

## Bounded framework and corpus inventory

`probe.py` reads `suite.json`, counts only non-reserved `.txt`/`.tsv` inputs and O1 address headers in fuzz-s1-200k. It prints v12/v13 as reserved without opening their data. There are 13 registered sets: 11 non-reserved with **110,551** addresses, and 2 reserved (not read). Fuzz O1 has **178,955 parsed addresses**; 142 recorded 21,045 invalid UTF-8 skipped inputs. Fuzz is a stress oracle and is not included in the 11-set suite aggregate.

The suite's integrity stage ran alone:

```sh
rtk python3 research/suite/run_suite.py --stages integrity --run-id 172-architecture-integrity
```

Exit 0. It hashed 11 datasets (110,551 addresses), 2 enabled variants, and 7 model files. No correctness, timing, memory or threads stages ran. The stage hashes model files but does not load them. Output and stderr are in `raw/suite-integrity.stdout` and `.stderr`; the run record is [run.log](/Users/kyle/Desktop/HW/CSCI%20201%20Reserach/research/suite/results/172-architecture-integrity/run.log) with full hashes in its `manifest.json`. Per-dataset counts appear in [raw/probe.stdout](raw/probe.stdout); suite/oracle/source hashes in [raw/source-hashes.txt](raw/source-hashes.txt).

## Falsifiable next probe

Populate the matrix for original and `cpi` from the integrity manifest and existing reports; mark unknowns explicitly. Add `161fx` only from its available evidence. This inventory probe did not measure parser behavior, speed or memory. Keep v12 unspent until a candidate passes screening and is selected.

## Decision

Adopt the hard-gates-first matrix for any future candidate comparison. It is a proposed evaluation policy, not a result about which parser is best. Current deliverable remains `cpi` per the handoff; `161fx` remains maintenance candidate pending the stated P4-style promotion decision.
