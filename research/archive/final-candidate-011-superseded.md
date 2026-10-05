# Final candidate (variant `h`, experiment 011) — summary for review

**Result:** about 2.7x the original libpostal parser throughput on one thread, byte-identical output, no extra memory, +3.7 s load time.

| Corpus | Baseline addr/s | Candidate addr/s | Speedup |
|---|---:|---:|---:|
| golden-v1, all 12,863 | 27,981 | 73,801 | 2.66x |
| novel set, 12,863 unseen strings | 27,322 | 73,746 | 2.71x |
| validation split (frozen harness) | 27,760 | 75,024 | 2.70x |

**Constraints verified:** byte-identical on golden-v1 and the novel set (12,863 each), on 4,000,000 fuzz-mutated inputs (baseline vs candidate result hashes identical) and by exhaustive/fuzz checks of every converted dictionary
(11.5M + 2.1M + 3.7M stored keys, 2M fuzz mutations each, 0 mismatches); one thread (ps -M) with CPU time equal to wall time; peak memory footprint 2.557 GB vs 2.613 GB baseline; steady footprint 2,438 MB.
**Limits:** the ASCII normalization fast path covers 58-60% of our datasets (the speedup would be about 2.36x with 0% and 2.84x with 100% eligible inputs, estimated); load time 2.9 s -> 6.6 s; measured on one shared, loaded laptop;
the baseline library itself crashes on some invalid UTF-8 (not changed); the 3.5x target was NOT reached.
Files: `final/BUILD.md`, `final/patches/`, `final/sources/`; evidence in `experiments/003 ... 012`, methodology in `METHOD.md`.
