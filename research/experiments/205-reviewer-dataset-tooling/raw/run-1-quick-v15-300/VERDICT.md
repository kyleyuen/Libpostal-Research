# Verdict: PASS

Run `run-1-quick-v15-300`: `optimized` (optimized) compared with `original` (original libpostal) on the same inputs and machine.

| Check | Result | Detail |
|---|---|---|
| correctness | PASS | 1 dump(s) covering 300 addresses and 2 fuzz stream(s) byte-identical; negative control detected |
| speed | PASS | v15: 5.74x (95% CI None-None; per round 5.74-5.74) |
| memory | PASS | optimized peak RSS and peak footprint <= original in 1/1 same-round pairs |
| threads | PASS | optimized: 3 live sample(s), max 1 thread(s); raw samples [1, 1, 1, 0, 0, 0] (0 = process already finished) |

| Dataset | Warm speedup | Original µs/addr | Optimized µs/addr | Startup s (orig / opt) | Break-even addresses |
|---|---:|---:|---:|---|---:|
| v15 | 5.74× | 14.631 | 2.547 | 2.7 / 9.72 | 580,875 |

Warm speedup excludes startup. Break-even is how many addresses one process must parse before the optimized build's slower startup is repaid.
Full tables, per-country numbers and raw logs: `report.md`, `timing/`, `env.txt`.
