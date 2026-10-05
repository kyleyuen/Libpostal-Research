# Experiment 139 — interleaving plan, Phase 0: switch cost and memory-level parallelism on this Mac (microbenchmark)

Date/time/timezone: created 2026-10-04T18:07:13-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: complete — **gate passed**

Plan: `research/plans/INTERLEAVING_PLAN_2026-10-04.md`. Program `tools/phase0.c` (clang -O2), three runs (`raw/run1-3.stdout`). The machine was in interactive use (load average 6.5), which does not change the conclusion.
- **Hand-written arm64 user-space context switch** (callee-saved x19–x30, d8–d15, sp), measured by ping-pong: **9.6 / 9.6 / 10.6 ns per switch**, including the loop around it.
- **Random pointer chase over 512 MB, K independent chains in one loop:**

| K | ns per access | throughput vs K = 1 |
|---:|---:|---:|
| 1 | 116.4 | 1.0× |
| 2 | 57.9 | 2.0× |
| 4 | 28.6 | 4.1× |
| 8 | 14.2 | 8.2× |

- **The same chases as K coroutines** (prefetch the next node, switch, dereference on resume):

| K | ns per access | throughput vs K = 1 |
|---:|---:|---:|
| 1 | 116.9 | 1.0× |
| 2 | 58.5 | 2.0× |
| 4 | 29.6 | 3.95× |
| 8 | 15.6 | 7.5× |

Conclusion:
- **Switching:** the core overlaps at least 8 independent memory waits. Coroutine switching captures almost all of it; the switch cost hides behind the outstanding miss.
- **Gate:** switch ≤ 10 ns (met, about 10 ns) and 4 coroutine chains ≥ 2.5× the throughput of 1 (3.95×) → **Phase 1 may start**.
- **Caveat:** this is the best case. Every step is a guaranteed miss with no compute in between. In the parser, many loads hit cache and each address has real compute, so the gain will be far smaller (see the plan's model).
