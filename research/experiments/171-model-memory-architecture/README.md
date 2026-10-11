# Experiment 171 — model-loading and memory budget for new architectures

Date/time/timezone: 2026-10-05T22:34:39-0700 (workspace clock; environment metadata labels the work date 2026-10-06).
Status: **diagnostic design study; no candidate built or timed**.
Hardware/OS: Apple M4 Pro, macOS 26.5.2, Darwin 25.5.0 arm64. No compiler invoked.

## Hypothesis and scope

The practical design space is constrained by two different metrics: peak process footprint during setup and steady parser memory. `cpi`'s measured peak is about 2.543 GB vs the original 2.614 GB, while the record file experiment's `P = peak footprint + mapped file size` was about 2,189 MiB. The experiment also exposes a measurement caveat: clean file-backed mapped pages are excluded from macOS `phys_footprint` but contribute to RSS. A future architecture must compare consistent resident/physical metrics and account for transient construction buffers, not just serialized table size.

Memory candidates to compare on paper before implementation: (A) preserve original model/trie and replace only transient per-address scratch; (B) memory-map an offline compiled representation and release replaced structures promptly; (C) lazy page-in/offline tables with explicit first-parse fault accounting. Do not assume mmap is free or compare RSS against footprint.

## Evidence and risks

- 151/153–155: model-derived records fit the corrected replacement budget on accounting, but 155's timed masked arena left only about 15 MB projected headroom vs cpi structures; peak remained a projection.
- 159's simple phrase hash table was 599.5 MB vs about 232 MB of in-memory tries, and was slower. 160's 1,117 MB mapped record design had an indicative P around 2,189 MiB but was killed for speed; steady memory was explicitly not measured in 162 due a vmmap grep failure.
- 150/README: original peak around 2.614 GB; cpi around 2.543 GB; steady 1,927 vs 2,492 MB. cpi setup around 11.6 s vs 2.75 s original. These are historical references, not this study's measurements.
- A generated table built on the target may peak far above the parser budget (160 generator reported 3.38 GB footprint); offline generation/shipping adds disk, provenance and compatibility concerns.

## Bounded artifact probe

`probe.py` stats existing files without reading model/record contents or mapping them:

| Artifact | Bytes |
|---|---:|
| Experiment 160 record arena | 1,117,339,520 |
| Experiment 160 slot IDs | 54,228,388 |
| Existing generator executable | 719,880 |
| Preserved original `libpostal.a` | 13,641,784 |

Full stat output is [raw/probe.stdout](raw/probe.stdout). Exit 0; stderr empty. This directly confirms the compiled arena is a gigabyte-scale secondary artifact and cannot be treated as negligible metadata. It does not establish resident RAM. The historical 160 generator peak of 3.38 GB remains a source-record measurement, not a new probe result. No model was loaded and no memory/timing run occurred. Starting HEAD `181737d30977adc9d287e3f7a1cfafd7277525d4`; relevant source hashes: [raw/source-hashes.txt](raw/source-hashes.txt).

## Falsifiable next probe

Before implementation, define non-overlapping measures for peak footprint, max RSS, mapped resident bytes, anonymous resident bytes, steady footprint/RSS, setup, first-parse faults and disk bytes. This file-size probe is inconclusive on RAM. Next, run a bounded allocation test only after projected peak passes; kill above original minus 20 MB or if mapped residency cannot be compared fairly.

## Decision

Treat memory headroom as a hard feasibility gate, not as a secondary score. For the next architecture, prefer replacing/releasing structures over adding a parallel representation; require load time and first-parse cost to be reported. No memory savings are claimed here.
