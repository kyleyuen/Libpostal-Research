# Second-opinion audit — 2026-10-05

Auditor: Codex, reviewing the recorded libpostal work independently. This is
a one-time audit, separate from the student optimization experiments.

## Results

**Passed.** On a newly sourced 3,298-input holdout, 15 serial paired trials
gave **6.590× median warm parsing speedup** (lowest trial 6.125×) and a
lowest cold-pass speedup of 4.476×. Candidate peak RSS was 63.5–64.7 MB lower
in every pair; steady footprint was 1,929 MB versus 2,491 MB. Both parser
processes showed one thread. Ordered default-parser outputs were byte
identical on the holdout and on 45 additional international addresses; 1.5
million differential fuzz hashes also matched.

The measured parsing speedup exceeds the stated 3.5× target with one thread
and lower memory use. The result measures parsing after setup: candidate setup
took 8.799 seconds longer, so a small one-off batch is slower end to end.

## Separate library-quality finding

The candidate aborts (exit 134) on the public
`libpostal_parser_print_features(true)` path, while the original exits 0.
Default parsing, parser teardown/reinitialization and the tested disabled
integer-key lifecycle path all produced identical output. The candidate
therefore should not be represented as a complete drop-in replacement until
that public API regression is fixed. No library code was changed in this audit.

## Evidence

- [Full protocol, all 15 rounds and limitations](../experiments/150-independent-professor-audit/README.md)
- [Frozen independent dataset](../../profiling/independent-audit-v1/README.md)
- [Raw analysis](../experiments/150-independent-professor-audit/raw/analysis.json),
  [timing logs](../experiments/150-independent-professor-audit/raw/timing/),
  [fuzz manifest](../experiments/150-independent-professor-audit/raw/fuzz/manifest.json),
  [verification](../experiments/150-independent-professor-audit/raw/verification.json)

The audit checked source and binary hashes and saved its raw evidence under
experiment 150 to satisfy the repository's numbered-record protocol. The
initial 40-pass calibration files were overwritten before archival and were
excluded from the reported 15-round statistics; the full record discloses
this gap. The reserved v12/v13 datasets were not touched.
