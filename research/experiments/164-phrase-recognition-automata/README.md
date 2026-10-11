# Experiment 164 — diagnostic design study: phrase recognition automata

Date/time/timezone: 2026-10-05 22:37 PDT (workspace environment labels current date 2026-10-06; shell clock captured in `raw/evidence.txt`).
Status: inconclusive — bounded oracle/log probe completed; no candidate built/timed.

## Hypothesis and change

Opportunity: context-fill phrase search still walks dictionary/component/postal tries. The exact hash approach in exp 159 reproduced trie quirks but was slower than the original trie warm (about 2× on golden-v1 and 1.3× on OSM) and required 599.5 MB versus roughly 232 MB of in-memory tries. A different architecture worth screening is a model-derived compact transition automaton/minimized acyclic word graph with byte-class transitions and terminal payloads, retaining the original token-level restart/continuation behavior. It may share suffix structure more effectively than per-prefix hash entries and reduce random probes.

This is a design hypothesis only. Exp 159 documented six observable search quirks; exp 156's optimistic hash bound was not exact on 0.25–0.97% of addresses. A safe architecture must encode the original state machine and return identical phrase arrays (start, length, data, order, boolean result), including empty/whitespace/ideographic edge cases. Memory must be measured against the actual in-memory structures replaced, not serialized file sizes. Likely risk: pointer/state traversal and transition lookup simply reproduce trie cost, while minimal-state construction or payload distinctions erase compression.

Falsifiable next probe: offline-build a small automaton from model-key dumps for dictionary/component/postal sets; prove per-key lookup equivalence and enumerate node/edge/payload bytes. Then replay saved token streams against original trie oracle before any parser wiring. Stop if footprint is not below replaced in-memory structures or exact restart semantics require a slow fallback on ordinary corpus tokens.

## Bounded corpus probe

Ran `python3 research/experiments/164-phrase-recognition-automata/probe.py golden-v1 osm-v3 fuzz-s1-200k` (exit 0; stdout/stderr/exit in `raw/probe.*`). The script counted O1 address headers and O2 token rows, then joined them to saved exp 159 exact phrase-search checks. Input, output, and script hashes: `raw/input_script_hashes.txt`.

| Corpus | O1 addresses | O2 addresses / tokens | Exact phrase calls | Reference fallbacks | Mismatched addresses |
|---|---:|---:|---:|---:|---:|
| golden-v1 | 12,863 | 12,863 / 75,659 | 38,589 | 3 | 0 |
| osm-v3 | 11,405 | 11,405 / 61,157 | 34,215 | 0 | 0 |
| fuzz-s1-200k | 178,955 | 178,919 / 1,363,734 | 536,757 | 21 | 0 |

The 36 fuzz IDs absent from O2 are enumerated in stdout (first examples: 84, 6504, 8020, 12539). Exp 142 explains O2/O3/O4 omit addresses taking the whole-string single-phrase shortcut; exact-search address count matches O1 for all three datasets. Thus fuzz comparisons align by O1 coverage, not O2 rows. This validates saved exact-search result scope but does not test a minimized automaton.

## Reproduction and provenance

Hardware/OS: Apple Silicon arm64, Darwin 25.5.0 (`raw/evidence.txt`). Compiler/build: none; no model loaded, build, or benchmark. Base commit: `181737d30977adc9d287e3f7a1cfafd7277525d4`. cpi source SHA-256: `address_parser.c` `a3db12f9d2daf32903f986197a1738eef5d383a5528d94e8e8100b0804f18536`; `feature_dict.h` `00eec5160f2771d3a48df8a37fa5418f1f23b6782be431d0766950a8ddc4c0e2`. Command output in `raw/evidence.txt`.

## Trials and correctness

No candidate, model, binary, new phrase-array correctness run, or benchmark was performed. This probe read O1/O2 and saved exp 159 outputs. No speed/memory claim. Acceptance requires byte-identical output on every golden-v1 input and single-thread parsing. No frozen reference or historical artifact was changed.

## Decision

Inconclusive. Search volume/fallback counts and aligned coverage are data-backed, but there is no compact automaton size/equivalence result. Next action is representation-only sizing and exact key/phrase-array checks from preserved key dumps; integration requires exactness and a footprint pass. Ledger/report/activity log intentionally untouched per task scope.
