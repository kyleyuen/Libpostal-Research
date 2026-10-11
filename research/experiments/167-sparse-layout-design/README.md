# Experiment 167 — sparse weight and feature data-layout alternatives

Date/time/timezone: 2026-10-05, America/Los_Angeles (bounded diagnostic probe; raw command evidence is in `raw/`).  
Status: **diagnostic design study; no candidate built or timed.**

## Hypothesis and scope

Changing sparse-row layout might reduce memory traffic enough to improve cpi without changing row values or emission order. This probe tests that premise with existing model counters and actual original feature-hit streams, rather than a new runtime.

`tools/probe.sh` hashes the evidence, extracts the direct model counters from experiment 135 and record accounting from 153, and counts hit/miss decisions from all four immutable O3i corpora. It does not load the model or consume reserved datasets.

## Bounded data-backed probe and findings

The model has 33,799,350 state and 13,369,573 transition weight records. Exact value sharing is weak: 65.5% of state values and 93.1% of transition values are distinct; no sampled value is float32-exact. A dictionary of values adds a large extra random access and its tables consume approximately 169 MB state plus 95 MB transition. Rows already average about 1.22 state and 1.49 transition 128-byte cache lines.

Dense transition storage is also ruled out by experiment 087: used rows average 74–77 of 400 cells and none reaches 200. The record compiler has already preserved every model row bit-for-bit, but its larger keyed arena/index makes each lookup more expensive. The direct O3i counters in this record give the hit/miss population that any replacement must preserve. Experiment 162 then measured the end-to-end consequence: 160b/160c were 17–34% slower than cpi warm despite passing their memory accounting.

## Why prior compiled records failed

They removed dictionaries, packed rows, `indptr`, and vocabulary lookup, but replaced a short sparse row walk with record hashing, index probes, key comparisons, slot scans, and feature-path control flow. The static model data leaves little lossless payload compression to pay for those new operations. This is an evidence-based explanation, not a microarchitectural measurement.

## Constraints, next probe, and decision

All alternatives must retain f64 bits in original emission order, byte-identical output, one parsing thread, model-only derivation, no cross-address cache, and peak/steady memory within the original. The raw probe is falsifiable through its source hashes and O3i counts.

Next falsifiable probe: only a layout that removes a measured operation rather than adding an indirection should proceed, with static byte accounting first, O3i equality on v1, then a pre-registered cpi comparison. **Decision: reject value dictionaries, dense rows, and the 160 keyed-record layout as further work.**
