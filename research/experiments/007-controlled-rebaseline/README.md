# Experiment 007 — Controlled re-baseline under an interference-aware protocol

Date: 2026-09-29 PDT (16:19-16:42). Status: **complete; earlier conclusions confirmed**
Purpose: the laptop is shared and noisy (several processes pinned near 100% CPU, load average ~6.5, swap ~42.7 of 44 GB used, battery charging). The user asked to redo the baseline and reconsider the earlier experiments with
environment controls (CPU load, battery/power state). Baseline and all candidates (003, 005, 006) were measured in ONE interleaved session so no old number is mixed with new conditions.

## Method (METHOD.md, "Environment control protocol")
- Hard gate before the session and before every run: AC power, Low Power Mode off, no thermal/performance warning (`research/tools/env_gate.sh`). Passed every time (0 aborts).
- Harness `research/tools/bench_ext.c`: `QOS_CLASS_USER_INTERACTIVE` for the benchmark thread (identical for all variants), single thread, model warmed on the OTHER dataset (one untimed pass), then 20 timed passes; per-pass wall time, CPU time, involuntary context switches;
  per-country times; setup seconds. Run under `caffeinate -i`, `/usr/bin/time -l`, with an environment snapshot before/after every run and a `top` CPU sampler during it (`raw/session1/*.env`, `*.cpu`).
- 6 processes per variant per dataset (golden-v1/all and the novel set), variant order rotated, 8 s gaps; 60 processes, 114 warm passes per variant per dataset. Analysis: `research/tools/analyze_quiet.py`, `tools/analyze_env.py`.

## Environment during the session (`raw/summary-env.md`)
Peak memory footprint per process (bytes) by variant (all processes, both datasets):

| variant | processes | min | median | max | max RSS max |
|---|---:|---:|---:|---:|---:|
| baseline | 12 | 2,613,037,264 | 2,613,299,432 | 2,613,545,192 | 2,614,394,880 |
| c003 | 12 | 2,613,168,360 | 2,613,545,180 | 2,614,069,456 | 2,614,935,552 |
| a5 | 12 | 2,612,709,584 | 2,613,160,168 | 2,613,987,560 | 2,614,853,632 |
| dn | 12 | 2,602,109,160 | 2,602,272,988 | 2,602,469,584 | 2,603,302,912 |
| d | 12 | 2,600,880,336 | 2,602,371,280 | 2,603,976,912 | 2,604,744,704 |

Environment during the session:
  batt: min 80.0 median 86.5 max 93.0 (n=120)
  busy_snap: min 35.2 median 40.3 max 55.8 (n=120)
  load: min 5.6 median 6.5 max 8.5 (n=120)
  swap_used_mb: min 42713.2 median 42753.2 max 42785.2 (n=120)
  battery health:           Charging: Yes;           State of Charge (%): 80;           Cycle Count: 210;           Condition: Normal;           Maximum Capacity: 95%;       Connected: Yes;       Charging: Yes;
Long-running processes not started by this work were burning about four cores throughout (two VS Code helpers up 26-28 days, Image Capture `icdd`, an unnamed `tmp.../t`); 81-89% of passes saw at least one involuntary context switch.
I did not stop the user's processes. Because variants were interleaved and the estimators below discount preempted passes, the comparisons are still fair.
System-busy % from the during-run `top` sampler was not captured (its output was lost when the sampler was killed; the before/after snapshots above are used instead).

## Results (`raw/summary-quiet.md`)
| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 114 | 3.01 | nan | 89% | 36.60 | 35.27 | 36.54 | 35.40 (13) | 36.77 |
| novel | c003 | 114 | 2.96 | nan | 89% | 31.81 | 30.53 | 31.78 | 30.41 (12) | 32.64 |
| novel | a5 | 114 | 2.99 | nan | 88% | 30.45 | 29.52 | 30.44 | 29.59 (14) | 31.53 |
| novel | dn | 114 | 6.04 | nan | 69% | 21.07 | 20.57 | 21.06 | 20.71 (35) | 21.22 |
| novel | d | 114 | 6.17 | nan | 82% | 20.33 | 19.85 | 20.32 | 19.86 (21) | 21.19 |
| v1-all | baseline | 114 | 2.94 | nan | 83% | 35.74 | 34.68 | 35.74 | 34.94 (19) | 36.47 |
| v1-all | c003 | 114 | 3.00 | nan | 88% | 31.32 | 30.32 | 31.31 | 30.39 (14) | 31.63 |
| v1-all | a5 | 114 | 2.97 | nan | 89% | 30.16 | 29.28 | 30.15 | 29.15 (13) | 30.22 |
| v1-all | dn | 114 | 5.96 | nan | 82% | 21.25 | 20.68 | 21.24 | 20.77 (20) | 21.38 |
| v1-all | d | 114 | 6.05 | nan | 81% | 20.10 | 19.60 | 20.09 | 19.75 (22) | 20.42 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | c003 | 1.151x | 1.156x | 1.150x | 1.164x | 1.126x |
| novel | a5 | 1.202x | 1.195x | 1.200x | 1.197x | 1.166x |
| novel | dn | 1.737x | 1.715x | 1.735x | 1.709x | 1.733x |
| novel | d | 1.800x | 1.777x | 1.798x | 1.783x | 1.735x |
| v1-all | c003 | 1.141x | 1.144x | 1.141x | 1.150x | 1.153x |
| v1-all | a5 | 1.185x | 1.185x | 1.185x | 1.199x | 1.207x |
| v1-all | dn | 1.682x | 1.677x | 1.682x | 1.682x | 1.706x |
| v1-all | d | 1.778x | 1.770x | 1.778x | 1.769x | 1.786x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   c003  : countries=17 min 1.091 (tw) median 1.123 max 1.150 (nl); below 1.05x: 0
  novel   a5    : countries=17 min 1.147 (lt) median 1.168 max 1.180 (ch); below 1.05x: 0
  novel   dn    : countries=17 min 1.550 (tw) median 1.650 max 1.794 (ru); below 1.05x: 0
  novel   d     : countries=17 min 1.654 (lt) median 1.757 max 1.930 (ru); below 1.05x: 0
  v1-all  c003  : countries=17 min 1.113 (ru) median 1.142 max 1.170 (hu); below 1.05x: 0
  v1-all  a5    : countries=17 min 1.160 (pl) median 1.185 max 1.220 (gb); below 1.05x: 0
  v1-all  dn    : countries=17 min 1.549 (lt) median 1.666 max 1.774 (ru); below 1.05x: 0
  v1-all  d     : countries=17 min 1.617 (lt) median 1.732 max 1.895 (ru); below 1.05x: 0

## Reconsidering the earlier experiments
- **Baseline (000/002)**: the controlled baseline is 35.74 us/address (27,981 addr/s) on golden-v1/all and 27,826 addr/s on validation, vs 27,821 (000) and 28,210 median (002) earlier: the frozen baseline holds within ~1% despite the noisy machine.
- **003 (batching)**: 1.141x (golden-v1) / 1.151x (novel) here vs 1.11-1.14x earlier: confirmed.
- **005 (exact work bundle)**: 1.185x / 1.202x vs baseline, +3.8% / +4.5% over 003 here vs +4.4% earlier: confirmed as a small real gain.
- **004 (interleaved trie walks, rejected)**: not re-run. Its conclusion (no gain over 003 on the trie) stands; note that prefetching *does* pay off with the dictionary (batched 40 ns vs 78 ns per lookup in the prototype), so explicit batching is now part of 006.
- **Memory**: the earlier RSS-based memory statements were unreliable under swap; footprint is now the metric. Peak footprint: baseline 2.613 GB, `d` 2.602 GB.

## Limitations
Single machine, shared and loaded; battery was charging (80% -> 93%) which can change thermal behaviour slightly; no root, so no core pinning or `powermetrics` frequency data; `top`-during-run CPU % missing. Absolute numbers apply to this laptop.

## Decision
Baseline re-established under the controlled protocol; all earlier conclusions confirmed; 006 adopted as the current best. Ledger, report and activity log updated: yes.
