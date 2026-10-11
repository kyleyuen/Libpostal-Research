# Experiment 170 — parser architecture independent of cpi

Date/time/timezone: 2026-10-05T22:34:39-0700 (workspace clock; environment metadata labels the work date 2026-10-06).
Status: **diagnostic design study; no candidate built or timed**.
Hardware/OS: Apple M4 Pro, macOS 26.5.2, Darwin 25.5.0 arm64. No compiler invoked.

## Hypothesis and scope

`cpi` has already exhausted useful local changes to the string-key/trie architecture: 146–149's integer keys did not speed it up, 139/140 interleaving regressed, and 159/160's full compiled-record and hash-search redesigns were killed. A different viable direction is a **model-derived transducer pipeline**: normalize/tokenize once; walk the existing libpostal phrase tries; emit feature-family events into bounded per-address scratch; then score and decode in one streaming pass. This keeps trie semantics and removes repeated feature-key allocation/hash/probe, without depending on cpi's packed-row code or the killed compiled-record grouping.

Other architecture families considered: (1) keep original tries and feature semantics but fuse event production with scoring via a small family-specific dispatch table; (2) model-derived minimized automaton for phrase/context searches; (3) compile the whole model into an offline feature transducer. The latter two are higher risk: 156 documents trie quirks, and 153–162 show record formats/lookup architecture can be slower even when exact and memory-feasible.

## Evidence and boundaries

- 141 says 60–68% of cpi cold time is front-half work; 151 estimates much of remaining instructions arise from key construction/probes/trie walks. These are motivation only.
- 157 bounds the feature-function control work at 0.923/0.596 µs (golden-v1/OSM), but 160's records in cpi were 29–34% slower (160b) and 17–18% slower (160c) than cpi warm. Reuse neither projected gains nor those records unchanged.
- 156's hash phrase-search bound was not exact: 0.25–0.97% address mismatch, caused by trie continuation/tail-compression behavior. 159 fixed exactness but was slower than trie search. Keep the existing tries for the first falsifiable probe.
- Public API parity and lifecycle matter. 150 found feature-print abort; 158/161 repair the defects, and 162 shows speed parity. Any architecture must support the exported parser APIs and setup/teardown behavior.

## Bounded oracle probe

`probe.py` streams O3i records and groups the original ordered feature events by family, counting misses, distinct hit IDs and per-address event volume.

| Oracle | Addresses with O3i | Events | Mean/address | p95 | Max | P events / hits | S events / hits |
|---|---:|---:|---:|---:|---:|---:|---:|
| golden-v1 | 12,860 | 702,370 | 54.617 | 91 | 286 | 112,356 / 107,117 | 590,014 / 496,259 |
| novel | 12,854 | 713,995 | 55.547 | 87 | 298 | 111,006 / 104,864 | 602,989 / 496,541 |
| OSM v3 | 11,405 | 546,890 | 47.952 | 77 | 224 | 88,492 / 85,889 | 458,398 / 409,873 |
| fuzz s1 | 177,099 | 14,030,294 | 79.223 | 179 | 1,146 | 2,198,416 / 2,098,883 | 11,831,878 / 9,639,199 |

O3i omits whole-string shortcut addresses. State events are ≈84–86% of emitted records; order and duplicate events must be preserved. Fuzz's maximum 1,146 events/address means a fixed common-case buffer needs an exact overflow path. This is structural evidence, not a speed result.

Exact command and four input paths are in [raw/commands.txt](raw/commands.txt); exit 0, stderr empty, stdout [raw/probe.stdout](raw/probe.stdout). No build, model load or timing. Starting HEAD `181737d30977adc9d287e3f7a1cfafd7277525d4`; oracle manifest and source hashes: [raw/source-hashes.txt](raw/source-hashes.txt).

## Falsifiable next probe

On a scratch copy, trace and consume original `(family,index,id)` events while retaining trie/context code. Compare event sequence, O4 score bits and final outputs on all four existing oracles. Kill on any mismatch, API/lifecycle deviation, or persistent memory above original. The observed overflow limit is 1,146 events/address. No speed claim until a full candidate is measured.

## Decision

Recommend the low-risk fused-event diagnostic as the next parser-architecture probe. Do not build an automaton or reuse 159/160 designs before that result. No speed or memory improvement is claimed. All candidate validation must preserve byte identity and single-thread parsing.
