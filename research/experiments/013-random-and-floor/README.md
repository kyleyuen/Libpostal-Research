# Experiment 013 — Speedup on the full real set, on random subsets, and on the worst-case inputs

Date: 2026-09-30. Status: **complete (measurement only; no library change)**
Question (user): what is the definite speedup on a random dataset, using ALL addresses and not a skewed subset?

## Method
Real, unmodified golden-v1 addresses only (the synthetic novel set is not used). `research/tools/bench_ext.c` (highest scheduling class, AC gate), baseline library vs the current best candidate `h` (experiment 011), each process:
setup, one untimed warm-up pass on a different file, then 15 timed passes. Alternating order, 2-3 processes per variant per set. Sets (`data/`, built by the runner script from `profiling/golden-v2-novel/v1-all.tsv`):
the full set (12,863); `floor_non_ascii_only` (5,404 addresses that the ASCII normalization fast path cannot help: any non-ASCII byte, `&` or control character); `ascii_only` (7,459); eight unbiased random samples of 2,000 addresses (seeded, 56-60% ASCII, matching the whole).
A first attempt of the runner had a quoting bug (paths with spaces) and produced garbage runs; it was killed and its output kept in `raw/discarded_broken_attempt_unquoted_paths`.

## Results (`raw/summary.md`)
| Set | Addresses | Baseline µs/addr | Candidate µs/addr | Speedup (median) | Speedup (P10) |
|---|---:|---:|---:|---:|---:|
| **Full real set** | 12,863 | 34.88 | 13.22 | **2.638x** | 2.628x |
| random subset 1 | 2,000 | 34.76 | 12.86 | 2.703x | 2.807x |
| random subset 2 | 2,000 | 34.62 | 12.88 | 2.688x | 2.745x |
| random subset 3 | 2,000 | 34.65 | 12.77 | 2.713x | 2.736x |
| random subset 4 | 2,000 | 34.64 | 12.98 | 2.669x | 2.757x |
| random subset 5 | 2,000 | 34.80 | 12.88 | 2.701x | 2.701x |
| random subset 6 | 2,000 | 34.64 | 12.91 | 2.683x | 2.727x |
| random subset 7 | 2,000 | 35.05 | 13.01 | 2.694x | 2.746x |
| random subset 8 | 2,000 | 35.03 | 13.09 | 2.677x | 2.648x |
| ASCII-only inputs | 7,459 | 33.06 | 11.38 | 2.906x | 2.903x |
| **Worst case: only non-ASCII / `&` / control inputs** | 5,404 | 34.16 | 14.67 | **2.329x** | 2.313x |

Eight random 2,000-address samples of the real set: min 2.669x, median 2.691x, max 2.713x.

## Reading
- The speedup on the whole real set is **2.64x** (this session) and stays inside 2.67-2.71x on random samples of the same data.
- No input is slower than the baseline. The **measured floor is 2.33x**, on the 5,404 real addresses for which the ASCII normalization path never applies; ASCII inputs reach 2.91x.
- A dataset with a very different ASCII share would land between those two numbers (about 2.33x with none, about 2.9x with all). What the ASCII fast path changes is therefore worth at most +0.6x, and 2.33x is the number that does not depend on it.
- Limits: one machine, shared and loaded; the whole real set is public OSM-derived test data with 88 countries, so a truly different distribution (for example synthetic random strings) is not covered.
