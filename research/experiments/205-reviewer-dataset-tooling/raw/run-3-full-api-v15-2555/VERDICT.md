# Verdict: INCONCLUSIVE

Run `run-3-full-api-v15-2555`: `optimized` (optimized) compared with `original` (original libpostal) on the same inputs and machine.

| Check | Result | Detail |
|---|---|---|
| correctness | PASS | 1 dump(s) covering 2,555 addresses and 2 fuzz stream(s) byte-identical; negative control detected |
| speed | PASS | v15-tooling-check: 7.35x (95% CI 5.59-9.66; per round 6.88-8.34) |
| memory | INCONCLUSIVE | optimized peak RSS and peak footprint <= original in 1/3 same-round pairs; v15-tooling-check: peak RSS +238,567,424 B while the original was paged (its RSS/footprint 0.69; footprint lower); v15-tooling-check: peak RSS +90,783,744 B while the original was paged (its RSS/footprint 0.73; footprint lower); INCONCLUSIVE: the machine was paging memory during the run - free memory (close apps) and rerun |
| threads | PASS | optimized: 6 live sample(s), max 1 thread(s); raw samples [1, 1, 1, 1, 1, 1] (0 = process already finished) |
| api | KNOWN LIMITATION | 7/8 public-API modes identical on 2,555 addresses; FAILED: print_features; toponym: both builds exit -11 identically after 189 lines (upstream behavior, reproduced) |

KNOWN LIMITATION: `libpostal_parser_print_features(true)` (a debugging switch that prints internal parser features) makes the optimized build stop with an error by design; every other API family must match. See research/experiments/204-public-api-parity/README.md.

| Dataset | Warm speedup | Original µs/addr | Optimized µs/addr | Startup s (orig / opt) | Break-even addresses |
|---|---:|---:|---:|---|---:|
| v15-tooling-check | 7.35× | 32.26 | 4.658 | 3.18 / 10.23 | 255,344 |

Warm speedup excludes startup. Break-even is how many addresses one process must parse before the optimized build's slower startup is repaid.
Full tables, per-country numbers and raw logs: `report.md`, `timing/`, `env.txt`.
