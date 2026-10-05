# Experiment 041 — warm bound: how fast is `l3` when each address's data is already cached? (diagnostic)

Date/time/timezone: created 2026-10-02T17:01:33-0700; run 2026-10-02T17:01:55-0700 to 2026-10-02T17:03:07-0700, America/Los_Angeles.
Status: complete (measurement only)

## Method
(1) Superpage test (`tools/superpage.c`): `mach_vm_allocate` with `VM_FLAGS_SUPERPAGE_SIZE_ANY` / `_2MB` and the mmap form all fail with "invalid argument" on this macOS/arm64 machine (`raw/superpage.stdout`); base page size 16 KB. Larger pages are not available to user code.
(2) Diagnostic harness `tools/bench_warm.c` (no library change) linked against the unchanged `l3` library with its PGO profile, LTO and mimalloc (`raw/hashes.txt`): every address is parsed once untimed and then again, timing only the second call. Compared with the normal `l3` bench_ext; golden-v1 (`v1-all.tsv`), 10 passes, 3 alternating runs (`tools/run.sh`, all exit 0). Median of warm passes 2–10.

## Results
| run | warm second call µs/addr | first call of the pair | normal bench_ext |
|---|---:|---:|---:|
| r1 | 3.666 | 8.054 | 7.838 |
| r2 | 3.665 | 8.077 | 7.824 |
| r3 | 3.666 | 8.136 | 7.877 |

Components identical to normal runs (550,370 over 10 passes). The full, unmodified `l3` pipeline costs **3.67 µs/address (≈9.2× the original's ≈33.7 µs) when the address's model data is cached**, versus 7.84 µs normally: **≈4.17 µs (53%) is first-touch memory cost.** Diagnostic only — re-parsing the same address is not a usable strategy.

## Consequence
The 7.8× target (≈4.3 µs) lies between the warm bound and today's time; it requires removing ≈85% of the first-touch cost with the current code, or less if instructions are also cut. The work is data layout (fewer distinct cache lines and pages per address), not instruction count. Known shares so far: phrase-trie searches ≈1.2 µs (038), feature generation ≈1.1 µs (040).
