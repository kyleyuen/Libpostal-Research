# Experiment 034 — can benchmark runs execute in parallel without distorting each other? (calibration)

Date/time/timezone: created 2026-10-02T15:50:31-0700; run 2026-10-02T15:50:31-0700 to 2026-10-02T15:55:50-0700, America/Los_Angeles.
Status: complete — **no** for timing; parallel use is restricted to builds, correctness checks and instruction counts.

## Question and setup

User request (2026-10-02): run several single-threaded tests at once to use the M4 Pro (8 P-cores in two 4-core clusters sharing a 16 MB L2 each, 4 E-cores, 24 GB RAM, ~12 GB swap already in use), provided simultaneous runs do not change each other's results. macOS cannot pin a process to a core; the harness requests QoS user-interactive. Test: the same preserved `l3` bench_ext on golden-v1 (`v1-all.tsv`, 20 passes, median of warm passes 2–20) run solo and as 2, 3 and 4 simultaneous copies, plus the original and `l3` run one after the other vs simultaneously; 3 rounds; vm_stat swap/pageout counters and memory pressure recorded around each group (`raw/vm.txt`). Command `caffeinate -i /bin/bash tools/run.sh`; analysis inline (`raw/analyze.stdout`).

## Results (`raw/analyze.stdout`)

    solo: per-process warm µs by round [[7.897], [7.984], [7.923]]  median 7.923
    k2: per-process warm µs by round [[8.45, 8.486], [8.427, 8.446], [8.43, 8.49]]  median 8.448
    k3: per-process warm µs by round [[8.759, 8.776, 8.771], [8.974, 8.968, 8.93], [9.007, 9.002, 8.991]]  median 8.968
    k4: per-process warm µs by round [[8.852, 8.841, 8.81, 8.827], [9.788, 9.383, 9.345, 9.285], [8.831, 8.827, 8.809, 8.87]]  median 8.846
    r1: sequential orig 34.397 l3 7.903 ratio 4.352 | simultaneous orig 36.727 l3 8.329 ratio 4.410
    r2: sequential orig 34.531 l3 8.011 ratio 4.310 | simultaneous orig 37.472 l3 8.353 ratio 4.486
    r3: sequential orig 34.280 l3 7.925 ratio 4.326 | simultaneous orig 36.985 l3 8.336 ratio 4.437

Per-process slowdown vs solo (median): 2 copies +6.6%, 3 copies +13.2%, 4 copies +11.7% (round 2 of k4 9.3–9.8 µs; swap-outs rose during k2/k3/k4 groups of round 1, `raw/vm.txt`). Original + `l3` simultaneously: speedup ratio 4.41/4.49/4.44× vs 4.35/4.31/4.33× sequential — contention slows the original (~+7%) more than `l3` (~+5%), inflating the ratio.

## Decision

Simultaneous processes share L2, memory bandwidth and RAM (each holds ~2.2–2.6 GB), so they distort both absolute times and paired ratios beyond the effects we measure (1–5%). Rule from now on: **timing runs stay one at a time**; builds, output/fuzz verification and instruction-count screens may run in parallel (up to 3 at once, memory permitting), since their results do not depend on contention. Instruction counts under contention are assumed unaffected (UNVERIFIED; to be spot-checked).
